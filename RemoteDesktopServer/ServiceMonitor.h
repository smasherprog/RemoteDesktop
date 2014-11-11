#ifndef SERVICE_MONITOR_H
#define SERVICE_MONITOR_H
#include <thread>
#include "../RemoteDesktop_Library/Utilities.h"

namespace RemoteDesktop{
	class ServiceMonitor{
		void _Run();
		void _LaunchProcess(std::ofstream& myfile);
		bool Running = false;
		bool StartProcess = false;

		STARTUPINFO          StartUPInfo;
		PROCESS_INFORMATION ProcessInfo;

		wchar_t szPath[MAX_PATH]; 
		
		typedef DWORD(*WTSGETACTIVECONSOLESESSIONID)();
		DynamicFn<WTSGETACTIVECONSOLESESSIONID> lpfnWTSGetActiveConsoleSessionId;
		std::thread _BackGroundNetworkWorker;
		DWORD _CurrentSession = 0xFFFFFFFF;
		DWORD _LastSession = 999;
	public:
		ServiceMonitor();
		~ServiceMonitor();
		void Start();
		void Stop();
		void LaunchProcess();
	};
};

#endif