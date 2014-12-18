#include "stdafx.h"
#include "Clipboard_Monitor.h"
#include "Desktop_Monitor.h"

RemoteDesktop::ClipboardMonitor::ClipboardMonitor(Delegate<void, const Clipboard_Data&> c) : _OnClipboardChanged(c) {
	_Running = true;
	_BackGroundWorker = std::thread(&RemoteDesktop::ClipboardMonitor::_Run, this);
}
RemoteDesktop::ClipboardMonitor::~ClipboardMonitor(){
	_Running = false;
	if (_BackGroundWorker.joinable()) _BackGroundWorker.join();
}
void RemoteDesktop::ClipboardMonitor::Restore(const Clipboard_Data& c){
	std::lock_guard<std::mutex> l(_ClipboardLock); 
	_IgnoreClipUpdateNotice = true;
	Clipboard::Restore(_Hwnd, c);
	_IgnoreClipUpdateNotice = true;
}
void RemoteDesktop::ClipboardMonitor::_Run(){
	DesktopMonitor dekstopmonitor;
	dekstopmonitor.Switch_to_ActiveDesktop();

	auto myclass = L"myclass";
	WNDCLASSEX wndclass = {};
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.lpszClassName = myclass;
	if (RegisterClassEx(&wndclass))
	{
		_Hwnd = CreateWindowEx(0, myclass, L"clipwatcher", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
		if (!AddClipboardFormatListener(_Hwnd)){
			DEBUG_MSG("Error %", GetLastError());
		}
	}
	else {
		DEBUG_MSG("Error %", GetLastError());
	}
	SetTimer(_Hwnd, 1001, 500, NULL); //every 500 ms windows will send a timer notice to the msg proc below. This allows the destructor to set _Running to false and the message proc to break
	MSG msg;
	while (_Running){
		if (PeekMessage(&msg, _Hwnd, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_TIMER)
			{
			}
			else if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY){
				break;//get out of the loop and destroy
			}
			else if (msg.message == WM_CLIPBOARDUPDATE){
				if (!_IgnoreClipUpdateNotice) {
					Clipboard_Data c;
					{//ensure lock is released timely
						std::lock_guard<std::mutex> l(_ClipboardLock);
						c = Clipboard::Load(_Hwnd);
					}
					_OnClipboardChanged(c);
				}
				_IgnoreClipUpdateNotice = false;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else WaitMessage();
	}
	if (_Hwnd != NULL) {
		RemoveClipboardFormatListener(_Hwnd);
		KillTimer(_Hwnd, 1001);
	}

}
