#include "stdafx.h"
#include "Clipboard.h"
#include "Clipboard_Wrapper.h"

const UINT formatUnicodeText = CF_UNICODETEXT;
const UINT formatRTF = RegisterClipboardFormat(L"Rich Text Format");
const UINT formatHTML = RegisterClipboardFormat(L"HTML Format");
const UINT formatDIB = CF_DIBV5;

bool RemoteDesktop::Clipboard::Load(void* hwnd, RemoteDesktop::Clipboard_Data& data){
	static Clipboard_Data LastClipboard;
	DEBUG_MSG("BEGIN Loading Clipboard");

	Clipboard_Wrapper clipwrap(hwnd);
	if (!clipwrap.IsValid()) return false;

	HANDLE hText = NULL;
	if (IsClipboardFormatAvailable(formatUnicodeText)) {
		hText = ::GetClipboardData(formatUnicodeText);
	}

	if (hText) {
		BYTE* pData = (BYTE*)GlobalLock(hText);
		int nLength = (int)GlobalSize(hText);

		if (pData != NULL && nLength > 0) {
			// Convert from UTF-16 to UTF-8
			int nConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, NULL, 0, NULL, NULL);

			if (nConvertedSize > 0) {
				data.m_pDataText.resize(nConvertedSize);

				int nFinalConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, data.m_pDataText.data(), nConvertedSize, NULL, NULL);
				if (nFinalConvertedSize > 0) data.m_pDataText.resize(nFinalConvertedSize);
				else data.m_pDataText.resize(0);
			}
			GlobalUnlock(hText);
		}
	}

	_INTERNAL::LoadClip(data.m_pDataRTF, ::GetClipboardData, formatRTF);
	_INTERNAL::LoadClip(data.m_pDataHTML, ::GetClipboardData, formatHTML);
	_INTERNAL::LoadClip(data.m_pDataDIB, ::GetClipboardData, formatDIB);
	if (data == LastClipboard) return false;//dont update clipboard, its the same as the last update.. could be we are connected to ourselves in a loop
	LastClipboard = data;
	DEBUG_MSG("END Loading Clipboard");
	return true;
}

void RemoteDesktop::Clipboard::Restore(void* hwnd, const Clipboard_Data& c){
	static Clipboard_Data LastClipboard;
	if (c == LastClipboard) return;//dont update clipboard, its the same as the last update.. could be we are connected to ourselves in a loop
	DEBUG_MSG("BEGIN Restore Clipboard");
	Clipboard_Wrapper clipwrap(hwnd);
	if (!clipwrap.IsValid()) return;
	if (!::EmptyClipboard()) return;

	if (!c.m_pDataText.empty()){
		int nConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)c.m_pDataText.data(), c.m_pDataText.size(), NULL, 0);
		if (nConvertedSize > 0) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, nConvertedSize * sizeof(wchar_t));
			if (hData) {
				BYTE* pData = (BYTE*)GlobalLock(hData);
				if (pData) {
					int nFinalConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)c.m_pDataText.data(), c.m_pDataText.size(), (LPWSTR)pData, nConvertedSize);
					GlobalUnlock(hData);
					if (nFinalConvertedSize > 0) {
						if (::SetClipboardData(formatUnicodeText, hData))
						{
							hData = NULL;
							DEBUG_MSG("BEGIN Restore Clipboard formatUnicodeText");
						}
					}
				}
			}
			if (hData) {
				GlobalFree(hData);
			}
		}
	}
	_INTERNAL::RestoreClip(c.m_pDataRTF, ::SetClipboardData, formatRTF);
	_INTERNAL::RestoreClip(c.m_pDataHTML, ::SetClipboardData, formatHTML);
	_INTERNAL::RestoreClip(c.m_pDataDIB, ::SetClipboardData, formatDIB);
	LastClipboard = c;//copy the contents 
	DEBUG_MSG("END Restore Clipboard");
}