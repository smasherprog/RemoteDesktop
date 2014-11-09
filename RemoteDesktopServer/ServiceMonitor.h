#ifndef SERVICE_MONITOR_H
#define SERVICE_MONITOR_H
#include <thread>

namespace RemoteDesktop{
	class ServiceMonitor{
		void _Run();
		bool Running = false;
		bool StartProcess = false;

		STARTUPINFO          StartUPInfo;
		PROCESS_INFORMATION ProcessInfo;

		wchar_t szPath[MAX_PATH];

		std::thread _BackGroundNetworkWorker;
	public:
		ServiceMonitor();
		~ServiceMonitor();
		void Start();
		void Stop();
		void LaunchProcess();
	};
};

#endif