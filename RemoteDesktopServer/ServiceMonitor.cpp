#include "stdafx.h"
#include "ServiceMonitor.h"
#include "ServiceHelpers.h"

RemoteDesktop::ServiceMonitor::ServiceMonitor(){
	GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));
}
RemoteDesktop::ServiceMonitor::~ServiceMonitor(){
	Stop();
}
void RemoteDesktop::ServiceMonitor::Start(){
	Stop();
	Running = true;
	_BackGroundNetworkWorker = std::thread(&ServiceMonitor::_Run, this);
}
void RemoteDesktop::ServiceMonitor::Stop(){
	Running = false;
	if (ProcessInfo.hProcess != 0) TerminateProcess(ProcessInfo.hProcess, 0);
	if (std::this_thread::get_id() != _BackGroundNetworkWorker.get_id()){
		if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	}

}

void RemoteDesktop::ServiceMonitor::LaunchProcess(){
	StartProcess = true;
}
void RemoteDesktop::ServiceMonitor::_Run(){

	while (Running){
		Sleep(3000);
		if (StartProcess){
			if (ProcessInfo.hProcess != 0) TerminateProcess(ProcessInfo.hProcess, 0);
			memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
			memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));
			StartUPInfo.lpDesktop = L"Winsta0\\default";
			StartUPInfo.cb = sizeof(STARTUPINFO);
			HANDLE winloginhandle = NULL;
			if (GetWinlogonHandle(&winloginhandle)){
				CreateProcessAsUser(winloginhandle, NULL, szPath, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &StartUPInfo, &ProcessInfo);
			}
			CloseHandle(winloginhandle);
			StartProcess = false;
		}

	}
}
