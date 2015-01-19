#include "stdafx.h"
#include "SystemTray.h"
#include "resource.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"

#define WM_SYSICON          (WM_USER + 1)
#define ID_TRAY_APP_ICON    WM_SYSICON+1
#define ID_TRAY_BALLOON		WM_SYSICON+2
#define ID_TRAY_APP_TIMER   WM_SYSICON+3
#define ID_TRAY_START       WM_SYSICON+4
HWND AboutHwnd = nullptr;

INT_PTR CALLBACK AboutDlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
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
		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			AboutHwnd = nullptr;
			return TRUE;
		}
	}

	return FALSE;
}
void RemoteDesktop::SystemTray::_ShowAboutDialog(){
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), Hwnd, AboutDlgProc);
}


RemoteDesktop::SystemTray::SystemTray() :
_SystemTrayIcon(RAIIHICON((HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0))),
Hmenu(RAIIHMENU(CreatePopupMenu())) {
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
}
RemoteDesktop::SystemTray::~SystemTray(){
	Stop();
	_Cleanup();
}
void RemoteDesktop::SystemTray::_Cleanup(){
	if (notifyIconData.hWnd != 0) {
		Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
		notifyIconData.hWnd = 0;
	}
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
	Hmenu = nullptr;
	_SystemTrayIcon = nullptr;
	_TrayIconCreated = false;

}
void RemoteDesktop::SystemTray::Start(Delegate<void> readycb){
	IconReadyCallback = readycb;
	_BackGroundThread = std::thread(&SystemTray::_Run, this);
}
void RemoteDesktop::SystemTray::Stop(){
	_Running = false;
	PostMessage(Hwnd, WM_QUIT, 0, 0);
	BEGINTRY
		if (std::this_thread::get_id() != _BackGroundThread.get_id() && _BackGroundThread.joinable()) _BackGroundThread.join();
	ENDTRY
}

UINT s_uTaskbarRestart = 0;
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CREATE) s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
	RemoteDesktop::SystemTray *c = (RemoteDesktop::SystemTray *)GetWindowLong(hWnd, GWLP_USERDATA);
	if (c == NULL)
		return DefWindowProc(hWnd, msg, wParam, lParam);
	return c->WindowProc(hWnd, msg, wParam, lParam);
}
void RemoteDesktop::SystemTray::_CreateIcon(HWND hWnd){
	if (!FindWindow(L"Shell_TrayWnd", 0))
		return;

	if (_TrayIconCreated) return;

	Hmenu = RAIIHMENU(CreatePopupMenu());
	_SystemTrayIcon = RAIIHICON((HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0));

	CallBacks.clear();//always clear any callbacks before creation of the icon
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hWnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = WM_SYSICON; //Set up our invented Windows Message

	notifyIconData.hIcon = _SystemTrayIcon.get();
	TCHAR szTIP[64] = TEXT("Remote Desktop Process");
	wcscpy_s(notifyIconData.szTip, szTIP);
	if (Shell_NotifyIcon(NIM_ADD, &notifyIconData)){
		_TrayIconCreated = true;

		if (IconReadyCallback){

			IconReadyCallback();//let creator know the items can be added to the menu
			AddMenuItem(L"About", DELEGATE(&RemoteDesktop::SystemTray::_ShowAboutDialog));
		}
	}
}


LRESULT RemoteDesktop::SystemTray::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == s_uTaskbarRestart){
		_TrayIconCreated = false;
		_CreateIcon(Hwnd);

	}
	switch (msg){
	case(WM_TIMER) :
		_CreateIcon(Hwnd);
		break;
	case(WM_QUIT) :
	case(WM_CLOSE) :
	case(WM_DESTROY) :
	case(WM_QUERYENDSESSION) :
	case(WM_ENDSESSION) :
						_Cleanup();
		_Running = false;
		break;
	case(WM_SYSICON) :
		if (lParam == WM_RBUTTONUP)
		{
			// Get current mouse position.
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			// TrackPopupMenu blocks the app until TrackPopupMenu returns
			UINT clicked = TrackPopupMenu(Hmenu.get(), TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);
			SendMessage(hWnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away.
			for (size_t i = 0; i < CallBacks.size(); i++){
				if (clicked == ID_TRAY_START + i){
					CallBacks[i]();//call the callback
					break;//  no more searching
				}
			}
		}
					 break;

	}
	return DefWindowProc(hWnd, msg, msg, lParam);
}
void RemoteDesktop::SystemTray::_Run(){
	_Running = true;
	dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);

	auto myclass = L"systrayclass";
	WNDCLASSEX wndclass = {};
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = myclass;

	if (RegisterClassEx(&wndclass)) Hwnd = CreateWindowEx(0, myclass, L"systraywatcher", 0, 0, 0, 0, 0, HWND_DESKTOP, 0, GetModuleHandle(NULL), 0);
	else return DEBUG_MSG("Error %", GetLastError());

	ShowWindow(Hwnd, SW_HIDE);
	SetWindowLongPtr(Hwnd, GWLP_USERDATA, (LONG_PTR)this);

	auto timer(RAIIHWNDTIMER(Hwnd, ID_TRAY_APP_TIMER, 500)); //every 1000 ms windows will send a timer notice to the msg proc below. This allows the destructor to set _Running to false and the message proc to break
	MSG msg;
	while (_Running && (GetMessage(&msg, Hwnd, 0, 0) != 0))
	{

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}
void RemoteDesktop::SystemTray::AddMenuItem(const wchar_t* itemname, Delegate<void> cb){
	AppendMenu(Hmenu.get(), MF_STRING, ID_TRAY_START + CallBacks.size(), itemname);
	CallBacks.push_back(cb);
}
void RemoteDesktop::SystemTray::Popup(const wchar_t* title, const wchar_t* message, unsigned int timeout){

	if (!_TrayIconCreated) return;

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = Hwnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uVersion = NOTIFYICON_VERSION;

	Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData);
	notifyIconData.uFlags = NIF_INFO;
	notifyIconData.uTimeout = timeout;
	wcscpy_s(notifyIconData.szInfo, message);
	wcscpy_s(notifyIconData.szInfoTitle, title);
	Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	notifyIconData.uVersion = 0;
	Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData);
}
