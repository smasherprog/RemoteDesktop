#include "stdafx.h"
#include "Clipboard.h"
#include "Desktop_Monitor.h"


RemoteDesktop::Clipboard::Clipboard(){
	_Running = true;
	_BackGroundWorker = std::thread(&RemoteDesktop::Clipboard::_Run, this);
}
RemoteDesktop::Clipboard::~Clipboard(){
	_Running = false;
	if (_BackGroundWorker.joinable()) _BackGroundWorker.join();

}
void RemoteDesktop::Clipboard::_Run(){
	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_ActiveDesktop();

	auto myclass = L"myclass";
	WNDCLASSEX wndclass = {};
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.lpszClassName = myclass;
	HWND _WindowHandle = NULL;
	if (RegisterClassEx(&wndclass))
	{
		_WindowHandle = CreateWindowEx(0, myclass, L"clipwatcher", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
		if (!AddClipboardFormatListener(_WindowHandle)){
			DEBUG_MSG("Error %", GetLastError());
		}
	}
	else {
		DEBUG_MSG("Error %", GetLastError());
	}
	SetTimer(_WindowHandle, 1001, 500, NULL); //every 500 ms windows will send a timer notice to the msg proc below. This allows the destructor to set _Running to false and the message proc to break
	MSG msg;
	while (_Running){
		if (PeekMessage(&msg, _WindowHandle, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
			}
			else if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY){
				break;//get out of the loop and destroy
			}
			else if (WM_CLIPBOARDUPDATE){
				DEBUG_MSG("gotclip update!!");
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else WaitMessage();
	}
	if (_WindowHandle != NULL) {
		RemoveClipboardFormatListener(_WindowHandle);	
		KillTimer(_WindowHandle, 1001);
		DestroyWindow(_WindowHandle);
	}

}
