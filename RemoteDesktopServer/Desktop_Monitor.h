#ifndef DESKTOPMONITOR123_H
#define DESKTOPMONITOR123_H

namespace RemoteDesktop{

	class DesktopMonitor{
		HDESK threaddesktop = NULL;
	public:
		DesktopMonitor();
		~DesktopMonitor();
		bool Is_InputDesktopSelected() const;
		void Switch_to_ActiveDesktop();
		void SimulateCtrlAltDel();

		HDESK m_hDesk = NULL; 
	};
};

#endif