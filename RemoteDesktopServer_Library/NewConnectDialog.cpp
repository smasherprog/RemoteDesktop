#include "stdafx.h"
#include "resource.h"
#include "NewConnectDialog.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"


LRESULT CALLBACK RemoteDesktop::NewConnect_Dialog::DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static RemoteDesktop::NewConnect_Dialog* ptr = nullptr;
	switch (Msg)
	{
	case WM_INITDIALOG:
		ptr = (RemoteDesktop::NewConnect_Dialog*)lParam;
		ptr->_Hwnd = hWndDlg;
		ptr->_UpdateText();
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			ptr->OnAllow(ptr->_Name);
			ptr->Close();
			return TRUE;
		case IDCANCEL:
			ptr->OnDeny(ptr->_Name);
			ptr->Close();
			return TRUE;
		}
		break;
	}
	return FALSE;

}
RemoteDesktop::NewConnect_Dialog::NewConnect_Dialog(){


}

RemoteDesktop::NewConnect_Dialog::~NewConnect_Dialog(){
	Close();

}

void RemoteDesktop::NewConnect_Dialog::Close(){
	if (_Hwnd) EndDialog(_Hwnd, 0);
	_Hwnd = nullptr;
	if (_HFont) DeleteObject(_HFont);
	_HFont = nullptr;
	BEGINTRY
		if (std::this_thread::get_id() != _DialogThread.get_id() && _DialogThread.joinable()) _DialogThread.join();
	ENDTRY
}
void RemoteDesktop::NewConnect_Dialog::_UpdateText(){
	HWND texthWnd = GetDlgItem(_Hwnd, IDS_STRINGNEWCONNECTION);
	_HFont = CreateFont(18, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	auto textodisplay = _Name + std::wstring(L" would like to connect to your computer.");

	if (texthWnd && _HFont)
	{
		SetWindowText(texthWnd, textodisplay.c_str());
		SendDlgItemMessage(_Hwnd, IDS_STRINGNEWCONNECTION, WM_SETFONT, (WPARAM)_HFont, TRUE);
	}
}
void RemoteDesktop::NewConnect_Dialog::Show(){
	Close();
	_DialogThread = std::thread(&RemoteDesktop::NewConnect_Dialog::_Run, this);
}
void RemoteDesktop::NewConnect_Dialog::Show(std::wstring name){
	_Name = name;
	Show();
}
void RemoteDesktop::NewConnect_Dialog::_Run(){

	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);
	//below function is blocking until the dialog EndDialog is called which is why a new thread is created
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG3), NULL, reinterpret_cast<DLGPROC>(DlgProc), (LPARAM)this);

}