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
		int get_InputDesktop() const;
		int get_ThreadDesktop() const;
		bool Is_InputDesktopSelected() const;

	};
};

#endif