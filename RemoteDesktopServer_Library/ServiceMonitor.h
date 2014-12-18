#ifndef SERVICE_MONITOR_H
#define SERVICE_MONITOR_H
#include <thread>
#include "../RemoteDesktop_Library/Utilities.h"
#include <memory>

namespace RemoteDesktop{

	class ServiceMonitor{

		std::shared_ptr<PROCESS_INFORMATION> _App;
		HANDLE ExitProgHandle = nullptr;
		void _Run();
		std::shared_ptr<PROCESS_INFORMATION> _LaunchProcess(wchar_t* args);
		std::shared_ptr<PROCESS_INFORMATION> _LaunchDeleteProcess();
		bool Running = false;
		bool StartProcess = false;
		
		std::thread _BackGroundNetworkWorker;
		DWORD _CurrentSession = 0xFFFFFFFF;
		DWORD _LastSession = 999;
	public:
		ServiceMonitor();
		~ServiceMonitor();
	
		void Start();
		void Stop();

	};
};

#endif