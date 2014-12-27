#include "stdafx.h"
#include "resource.h"
#include "AboutDialog.h"

HWND AboutHwnd = NULL;
LRESULT CALLBACK AboutDlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		AboutHwnd = hWndDlg;
		return TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			AboutHwnd = nullptr;
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void RemoteDesktop::ShowAboutDialog(){
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), NULL, reinterpret_cast<DLGPROC>(AboutDlgProc));
}
void RemoteDesktop::CloseAboutDialog(){
	if (AboutHwnd != nullptr) EndDialog(AboutHwnd, 0);
	AboutHwnd = nullptr;
}
