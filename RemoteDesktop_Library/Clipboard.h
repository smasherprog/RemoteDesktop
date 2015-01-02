#ifndef CLIPBOARD_DATA123_H
#define CLIPBOARD_DATA123_H
#include <vector>

namespace RemoteDesktop{
	struct Clipboard_Data{
		std::vector<char> m_pDataText;
		std::vector<char> m_pDataRTF;
		std::vector<char> m_pDataHTML;
		std::vector<char> m_pDataDIB;
	};
	inline bool operator==(const Clipboard_Data& left, const Clipboard_Data& right){
		if (left.m_pDataDIB.size() != right.m_pDataDIB.size()) return false;
		if (memcmp(left.m_pDataDIB.data(), right.m_pDataDIB.data(), left.m_pDataDIB.size()) != 0) return false;

		if (left.m_pDataRTF.size() != right.m_pDataRTF.size()) return false;
		if (memcmp(left.m_pDataRTF.data(), right.m_pDataRTF.data(), left.m_pDataRTF.size()) != 0) return false;

		if (left.m_pDataHTML.size() != right.m_pDataHTML.size()) return false;
		if (memcmp(left.m_pDataHTML.data(), right.m_pDataHTML.data(), left.m_pDataHTML.size()) != 0) return false;

		if (left.m_pDataDIB.size() != right.m_pDataDIB.size()) return false;
		if (memcmp(left.m_pDataDIB.data(), right.m_pDataDIB.data(), left.m_pDataDIB.size()) != 0) return false;
		return true;// they are the same
	}
	namespace Clipboard{
		bool Load(void* hwnd, Clipboard_Data& data);
		void Restore(void* hwnd, const Clipboard_Data& c);
		namespace _INTERNAL{
			template<class T>void RestoreClip(const std::vector<char>& buffer, T cb, UINT format){
				if (buffer.size() > 0) {
					HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, buffer.size());
					if (hData) {
						BYTE* pData = (BYTE*)GlobalLock(hData);
						if (pData) {
							memcpy(pData, buffer.data(), buffer.size());
							GlobalUnlock(hData);
							if (cb(format, hData)) hData = NULL;
						}
					}
					if (hData) {
						GlobalFree(hData);
					}
				}
			}
			template<class T>void LoadClip(std::vector<char>& buffer, T cb, UINT format){
				HANDLE h = NULL;
				if (IsClipboardFormatAvailable(format)) {
					h = ::GetClipboardData(format);
				}
				if (h) {
					BYTE* pData = (BYTE*)GlobalLock(h);
					int nLength = (int)GlobalSize(h);

					if (pData != NULL && nLength > 0) {
						buffer.resize(nLength);
						memcpy(buffer.data(), pData, nLength);
						GlobalUnlock(h);
					}
				}
			}
		}
	}
}

#endif