#ifndef SYSTEMTRAY_123_H
#define SYSTEMTRAY_123_H
#include <thread>

namespace RemoteDesktop{
	class SystemTray{
		HWND Hwnd;
		HMENU Hmenu;

		std::thread _BackGroundThread; 
		NOTIFYICONDATA notifyIconData;

		void _Run();
		void _CreateIcon(HWND hWnd);
		
	public:
		SystemTray(){}
		~SystemTray(){ Stop(); }
		void Start();
		void Stop();
		void Popup(const wchar_t* title, const wchar_t* message, unsigned int timeout);
		LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}

#endif