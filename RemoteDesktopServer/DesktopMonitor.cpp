#include "stdafx.h"
#include "Desktop_Monitor.h"
#include <fstream>
#include <thread>
#include "..\RemoteDesktop_Library\Desktop_Wrapper.h"
#include "Wtsapi32.h"

RemoteDesktop::DesktopMonitor::DesktopMonitor(){
	CADEventHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEvenRDCad");
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

void RemoteDesktop::DesktopMonitor::SimulateCtrlAltDel(){
	SetEvent(CADEventHandle);
}


RemoteDesktop::DesktopMonitor::~DesktopMonitor(){
	if (m_hDesk != NULL)CloseDesktop(m_hDesk);
	if (CADEventHandle != NULL) CloseHandle(CADEventHandle);
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

	Desktop_Wrapper inputdesktop(OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP));

	if (inputdesktop.get_Handle() == NULL) return true;


	DWORD dummy;
	char threadname[256];
	char inputname[256];

	if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) return false;
	if (!GetUserObjectInformation(inputdesktop.get_Handle(), UOI_NAME, &inputname, 256, &dummy)) return false;
	return strcmp(threadname, inputname) == 0;
}
std::string RemoteDesktop::DesktopMonitor::get_ActiveUser(){
	char* ptr = NULL;
	DWORD size = 0;
	if (WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTSGetActiveConsoleSessionId(), WTS_INFO_CLASS::WTSUserName, &ptr, &size)){
		auto name = std::string(ptr);
		WTSFreeMemory(ptr);
		return name;
	}
	return "";
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