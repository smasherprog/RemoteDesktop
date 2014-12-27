#ifndef SYSTEMTRAY_123_H
#define SYSTEMTRAY_123_H
#include <thread>
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"
#include "..\RemoteDesktop_Library\Delegate.h"

namespace RemoteDesktop{
	class SystemTray{
		HWND Hwnd = nullptr;
		HMENU Hmenu = nullptr;
		HICON _SystemTrayIcon = nullptr;
		bool _TrayIconCreated = false;
		std::thread _BackGroundThread; 
		NOTIFYICONDATA notifyIconData;
		
		void _Cleanup();
		void _Run();
		void _CreateIcon(HWND hWnd);
		std::vector<Delegate<void>> CallBacks;
		Delegate<void> IconReadyCallback;
		
	public:
		SystemTray();
		~SystemTray();
		void Start(Delegate<void> readycb);
		void Stop();

		void AddMenuItem(const wchar_t* itemname, Delegate<void> cb); //this should only be called once the icon is ready

		void Popup(const wchar_t* title, const wchar_t* message, unsigned int timeout);
		LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}

#endif