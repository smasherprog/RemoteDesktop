#include "stdafx.h"
#include "resource.h"
#include "ProxyConnectDialog.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"

std::wstring textodisplay;
HFONT font = nullptr;
RemoteDesktop::RAIIDIALOGBOX_TYPE DiagHwnd = RAIIDIALOGBOX(nullptr);
bool RemoteDesktop::ReverseConnectID_DialogOpen(){
	return DiagHwnd != nullptr;
}
void UpdateText(){
	HWND texthWnd = GetDlgItem(DiagHwnd.get(), IDC_STATIC2);

	if (texthWnd && font != nullptr)
	{
		SetWindowText(texthWnd, textodisplay.c_str()); 
		SendDlgItemMessage(DiagHwnd.get(), IDC_STATIC2, WM_SETFONT, (WPARAM)font, TRUE);
	}
}
LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{

	case WM_INITDIALOG:
		DiagHwnd = RAIIDIALOGBOX(hWndDlg);
		UpdateText();
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			DiagHwnd = nullptr;//let the destructor call enddiag
			return TRUE;
		}
		break;
	}

	return FALSE;
}
void RemoteDesktop::CloseReverseConnectID_Dialog(){
	DiagHwnd = nullptr;
	font = nullptr;
}
void RemoteDesktop::ShowReverseConnectID_Dialog(int id){

	textodisplay = std::wstring(L"Please, give this number to your technician: ");
	textodisplay += std::to_wstring(id);

	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);
	RemoteDesktop::RAIIHFONT_TYPE hFont = RAIIHFONT(CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, \
		DEFAULT_PITCH | FF_SWISS, L"Arial"));
	font = hFont.get();
	auto p = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, reinterpret_cast<DLGPROC>(DlgProc));
	CloseReverseConnectID_Dialog();
}