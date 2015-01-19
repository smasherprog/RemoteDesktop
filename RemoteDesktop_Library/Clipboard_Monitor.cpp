#include "stdafx.h"
#include "Clipboard_Monitor.h"
#include "Desktop_Monitor.h"
#include "Handle_Wrapper.h"

RemoteDesktop::ClipboardMonitor::ClipboardMonitor(Delegate<void, const Clipboard_Data&> c) : _OnClipboardChanged(c) {
	_Running = true;
	_BackGroundWorker = std::thread(&RemoteDesktop::ClipboardMonitor::_Run, this);
}

RemoteDesktop::ClipboardMonitor::~ClipboardMonitor(){
	_Running = false;
	BEGINTRY
		if (std::this_thread::get_id() != _BackGroundWorker.get_id() && _BackGroundWorker.joinable()) _BackGroundWorker.join();
	ENDTRY
}

void RemoteDesktop::ClipboardMonitor::set_ShareClipBoard(bool s){
	_ShareClipboard = s;
}

void RemoteDesktop::ClipboardMonitor::Restore(const Clipboard_Data& c){
	if (_ShareClipboard){
		DEBUG_MSG("Clipboard Restore");
		std::lock_guard<std::mutex> l(_ClipboardLock);
		_IgnoreClipUpdateNotice = true;
		Clipboard::Restore(_Hwnd, c);
		_IgnoreClipUpdateNotice = true;
	}
}

void RemoteDesktop::ClipboardMonitor::_Run(){
	DesktopMonitor dekstopmonitor;
	if (!dekstopmonitor.Is_InputDesktopSelected()) dekstopmonitor.Switch_to_Desktop(DesktopMonitor::DEFAULT);

	auto myclass = L"myclass";
	WNDCLASSEX wndclass = {};
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.lpszClassName = myclass;
	if (RegisterClassEx(&wndclass))
	{
		_Hwnd = CreateWindowEx(0, myclass, L"clipwatcher", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
	}
	else {
		DEBUG_MSG("Error %", GetLastError());
	}

	auto clipboardlistener(RAIICLIPBOARDLISTENER(_Hwnd));
	auto timer(RAIIHWNDTIMER(_Hwnd, 1001, 500)); //every 500 ms windows will send a timer notice to the msg proc below. This allows the destructor to set _Running to false and the message proc to break

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
				if (_ShareClipboard){
					DEBUG_MSG("Clipboard Update");
					if (!_IgnoreClipUpdateNotice) {
						Clipboard_Data c;
						bool update = false;
						{//ensure lock is released timely
							std::lock_guard<std::mutex> l(_ClipboardLock);
							update = Clipboard::Load(_Hwnd, c);
						}
						if (update) _OnClipboardChanged(c);
					}
					_IgnoreClipUpdateNotice = false;
				}
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else WaitMessage();
	}
}
