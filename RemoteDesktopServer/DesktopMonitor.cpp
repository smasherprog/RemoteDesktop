#include "stdafx.h"
#include "Desktop_Monitor.h"
#include <fstream>
#include <thread>

RemoteDesktop::DesktopMonitor::DesktopMonitor(){

}
void ClearKeyState(WORD key)
{
	BYTE keyState[256];
	GetKeyboardState((LPBYTE)&keyState);
	if (keyState[key] & 1)
	{
		INPUT inp;

		inp.type = INPUT_KEYBOARD;
		inp.ki.wVk = key;
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

		// Simulate the key being pressed
		SendInput(1, &inp, sizeof(INPUT));
		inp.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
		// Simulate it being release
		SendInput(1, &inp, sizeof(INPUT));
	}
}
void _SimulateCtrlAltDel(){
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());

	auto hdesk = OpenDesktop(L"Winlogon", 0, false,
		DESKTOP_CREATEMENU |
		DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE |
		DESKTOP_HOOKCONTROL |
		DESKTOP_JOURNALPLAYBACK |
		DESKTOP_JOURNALRECORD |
		DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP |
		DESKTOP_WRITEOBJECTS);

	// turn off capslock if on
	ClearKeyState(VK_CAPITAL);
	// Winlogon uses hotkeys to trap Ctrl-Alt-Del...
	PostMessage(HWND_BROADCAST, WM_HOTKEY, 0, MAKELONG(MOD_ALT | MOD_CONTROL, VK_DELETE));
	// Switch back to our original desktop
	if (old_desktop != NULL)
	{
		SetThreadDesktop(old_desktop);
		CloseDesktop(hdesk);
	}
}

void RemoteDesktop::DesktopMonitor::SimulateCtrlAltDel(){
	auto t = std::thread(_SimulateCtrlAltDel);
	t.join();//wait
}


RemoteDesktop::DesktopMonitor::~DesktopMonitor(){
	if (m_hDesk != NULL)CloseDesktop(m_hDesk);
}

//
//RemoteDesktop::Desktops RemoteDesktop::DesktopMonitor::GetDesktop(HDESK s){
//	if (s == NULL)
//		return Default;
//	DWORD needed = 0;
//	wchar_t new_name[256];
//	auto result = GetUserObjectInformation(s, UOI_NAME, &new_name, 256, &needed);
//	std::wstring dname = new_name;
//	std::transform(dname.begin(), dname.end(), dname.begin(), ::tolower);
//	if (!result)
//		return Default;
//	if (std::wstring(L"default") == dname)
//		return Default;
//	else if (std::wstring(L"screensaver") == dname)
//		return ScreenSaver;
//	else
//		return Winlogon;
//	
//}

bool RemoteDesktop::DesktopMonitor::Is_InputDesktopSelected() const{

	// Get the input and thread desktops
	HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());

	HDESK inputdesktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP);

	if (inputdesktop == NULL) return true;


	DWORD dummy;
	char threadname[256];
	char inputname[256];

	if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
		CloseDesktop(inputdesktop);
		return false;
	}
	if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
		CloseDesktop(inputdesktop);
		return false;
	}
	CloseDesktop(inputdesktop);
	return strcmp(threadname, inputname) == 0;
}
void RemoteDesktop::DesktopMonitor::Switch_to_ActiveDesktop(){
	auto threadsk = GetThreadDesktop(GetCurrentThreadId());
	HDESK desktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

	if (desktop == NULL) return;
	if (!SetThreadDesktop(desktop))
	{
		CloseDesktop(desktop);
		return;
	}
	if (m_hDesk) {
		CloseDesktop(m_hDesk);
	}
	m_hDesk = desktop;
}