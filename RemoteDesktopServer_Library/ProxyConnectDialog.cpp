#include "stdafx.h"
#include "resource.h"
#include "ProxyConnectDialog.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"

std::wstring textodisplay;
HFONT hFont = NULL;
HWND DiagHwnd = NULL;
bool RemoteDesktop::ReverseConnectID_DialogOpen(){
	return DiagHwnd != NULL;
}
void UpdateText(){
	HWND texthWnd = GetDlgItem(DiagHwnd, IDC_STATIC2);
	if (hFont != NULL) DeleteObject(hFont);
	hFont = NULL;
	hFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, \
		DEFAULT_PITCH | FF_SWISS, L"Arial");

	if (texthWnd)
	{
		SetWindowText(texthWnd, textodisplay.c_str()); 
		SendDlgItemMessage(DiagHwnd, IDC_STATIC2, WM_SETFONT, (WPARAM)hFont, TRUE);
	}
}
LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{

	case WM_INITDIALOG:
		DiagHwnd = hWndDlg;
		UpdateText();
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;
	}

	return FALSE;
}
void RemoteDesktop::CloseReverseConnectID_Dialog(){
	EndDialog(DiagHwnd, 0);
	DiagHwnd = NULL;
	if (hFont != NULL) DeleteObject(hFont);
	hFont = NULL;
}
void RemoteDesktop::ShowReverseConnectID_Dialog(int id){

	textodisplay = std::wstring(L"Please, give this number to your technician: ");
	textodisplay += std::to_wstring(id);

	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);

	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, reinterpret_cast<DLGPROC>(DlgProc));
	if (hFont != NULL) DeleteObject(hFont);
	hFont = NULL;
}