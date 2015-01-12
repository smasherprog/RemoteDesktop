#include "stdafx.h"
#include "resource.h"
#include "GatewayConnectDialog.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"


LRESULT CALLBACK RemoteDesktop::GatewayConnect_Dialog::DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static RemoteDesktop::GatewayConnect_Dialog* ptr = nullptr;
	switch (Msg)
	{
	case WM_INITDIALOG:
		ptr = (RemoteDesktop::GatewayConnect_Dialog*)lParam;
		ptr->_Hwnd = hWndDlg;
		ptr->_UpdateText();
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			ptr->Close();
			return TRUE;
		}
		break;
	}

	return FALSE;
}

RemoteDesktop::GatewayConnect_Dialog::GatewayConnect_Dialog(){


}

RemoteDesktop::GatewayConnect_Dialog::~GatewayConnect_Dialog(){
	Close();
}

void RemoteDesktop::GatewayConnect_Dialog::Close(){
	if (_Hwnd) EndDialog(_Hwnd, 0);
	_Hwnd = nullptr;
	if (_HFont) DeleteObject(_HFont);
	_HFont = nullptr;
	BEGINTRY
		if (std::this_thread::get_id() != _DialogThread.get_id() && _DialogThread.joinable()) _DialogThread.join();
	ENDTRY
}
void RemoteDesktop::GatewayConnect_Dialog::_UpdateText(){
	HWND texthWnd = GetDlgItem(_Hwnd, IDC_STATIC2);
	_HFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	auto textodisplay = std::wstring(L"Please, give this number to your technician: ");
	textodisplay += std::to_wstring(_ID);

	if (texthWnd && _HFont)
	{
		SetWindowText(texthWnd, textodisplay.c_str());
		SendDlgItemMessage(_Hwnd, IDC_STATIC2, WM_SETFONT, (WPARAM)_HFont, TRUE);
	}
}

void RemoteDesktop::GatewayConnect_Dialog::Show(){
	Close();
	_DialogThread = std::thread(&RemoteDesktop::GatewayConnect_Dialog::_Run, this);
}
void RemoteDesktop::GatewayConnect_Dialog::Show(int id){
	_ID = id;
	Show();
}
void RemoteDesktop::GatewayConnect_Dialog::_Run(){

	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);
	//below function is blocking until the dialog EndDialog is called which is why a new thread is created
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, reinterpret_cast<DLGPROC>(DlgProc), (LPARAM)this);

}