#ifndef DESKTOPMONITOR123_H
#define DESKTOPMONITOR123_H

namespace RemoteDesktop{
	enum Desktops
	{
		ScreenSaver,
		Winlogon,
		Default
	};
	class DesktopMonitor{
		HWINSTA m_hCurWinsta =NULL;
		HWINSTA m_hWinsta = NULL;
		HDESK m_hDesk = NULL; 

		Desktops GetDesktop(HDESK s);

	public:
		DesktopMonitor();
		~DesktopMonitor();
		Desktops GetActiveDesktop();
		bool SwitchDesktop(Desktops dname);

		Desktops Current_Desktop;
		
	};
};

#endif