#ifndef DESKTOPMONITOR123_H
#define DESKTOPMONITOR123_H

namespace RemoteDesktop{

	class DesktopMonitor{

		HDESK m_hDesk = nullptr; 
	public:
		enum Desktops{DEFAULT=1, WINLOGON=2, SCREENSAVER=4, INPUT=8};

		DesktopMonitor();
		~DesktopMonitor();
		bool Switch_to_Desktop(int desired_desktop);

		static int get_InputDesktop();
		static int get_ThreadDesktop();
		static bool Is_InputDesktopSelected();

	};
};

#endif