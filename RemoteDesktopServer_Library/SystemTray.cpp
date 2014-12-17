#include "stdafx.h"
#include "SystemTray.h"
#include "resource.h"

#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_EXIT_REMOVE	1003
#define ID_TRAY_BALLOON		1004
#define ID_TRAY_APP_TIMER   1005
#define WM_SYSICON          (WM_USER + 1)

void RemoteDesktop::SystemTray::Start(){
	_BackGroundThread = std::thread(&SystemTray::_Run, this);
}
void RemoteDesktop::SystemTray::Stop(){
	PostMessage(Hwnd, WM_QUIT, 0, 0);
	if (std::this_thread::get_id() != _BackGroundThread.get_id()){
		if (_BackGroundThread.joinable()) _BackGroundThread.join();
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	RemoteDesktop::SystemTray *c = (RemoteDesktop::SystemTray *)GetWindowLong(hWnd, GWLP_USERDATA);
	if (c == NULL)
		return DefWindowProc(hWnd, msg, wParam, lParam);
	return c->WindowProc(hWnd, msg, wParam, lParam);
}
void RemoteDesktop::SystemTray::_CreateIcon(HWND hWnd){
	
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hWnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = WM_SYSICON; //Set up our invented Windows Message
	
	notifyIconData.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	TCHAR szTIP[64] = TEXT("Remote Desktop Process");
	wcscpy_s(notifyIconData.szTip, szTIP);
	Shell_NotifyIcon(NIM_ADD, &notifyIconData); 
	ShowWindow(Hwnd, SW_HIDE);
}
LRESULT RemoteDesktop::SystemTray::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_TIMER)
	{
		return 0;
	}
	else if (msg == WM_QUIT || msg == WM_CLOSE || msg == WM_DESTROY){
		return 1;
	}
	else if (msg == WM_SYSICON)
	{
		if (lParam == WM_RBUTTONUP)
		{
			// Get current mouse position.
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			// TrackPopupMenu blocks the app until TrackPopupMenu returns
			UINT clicked = TrackPopupMenu(Hmenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);
			SendMessage(hWnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away.
			if (clicked == ID_TRAY_EXIT || clicked == ID_TRAY_EXIT_REMOVE){
				PostMessage(Hwnd, WM_QUIT, 0, 0);
			}
		}
	}
	return DefWindowProc(hWnd, msg, msg, lParam);
}
void RemoteDesktop::SystemTray::_Run(){

	auto myclass = L"systrayclass";
	WNDCLASSEX wndclass = {};
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = myclass;

	wndclass.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0);
	wndclass.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);

	if (RegisterClassEx(&wndclass)) Hwnd = CreateWindowEx(0, myclass, L"systraywatcher", 0, 0, 0, 0, 0, HWND_DESKTOP, 0, GetModuleHandle(NULL), 0);
	else return DEBUG_MSG("Error %", GetLastError());
	

	Hmenu = CreatePopupMenu();
	AppendMenu(Hmenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
	AppendMenu(Hmenu, MF_STRING, ID_TRAY_EXIT_REMOVE, TEXT("Exit and Remove"));
	_CreateIcon(Hwnd);

	SetWindowLongPtr(Hwnd, GWLP_USERDATA, (LONG_PTR)this);
	SetTimer(Hwnd, ID_TRAY_APP_TIMER, 500, NULL); //every 500 ms windows will send a timer notice to the msg proc below. This allows the destructor to set _Running to false and the message proc to break

	MSG msg;
	while (GetMessage(&msg, Hwnd, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	KillTimer(Hwnd, ID_TRAY_APP_TIMER);
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}

void RemoteDesktop::SystemTray::Popup(const wchar_t* title, const wchar_t* message, unsigned int timeout){
	NOTIFYICONDATA notifyIconData;
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
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
