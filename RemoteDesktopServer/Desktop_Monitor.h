#ifndef DESKTOPMONITOR123_H
#define DESKTOPMONITOR123_H

namespace RemoteDesktop{

	class DesktopMonitor{
		HDESK threaddesktop = NULL;
		HANDLE CADEventHandle = NULL;
		std::string _lastUser;
		int _LastSessionId = -1;

	public:
		DesktopMonitor();
		~DesktopMonitor();
		bool Is_InputDesktopSelected() const;
		void Switch_to_ActiveDesktop();
		void SimulateCtrlAltDel();
		std::string get_ActiveUser();

		HDESK m_hDesk = NULL; 
	};
};

#endif