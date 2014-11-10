#include "stdafx.h"
#include "ServiceMonitor.h"
#include "ServiceHelpers.h"
#include <fstream>

RemoteDesktop::ServiceMonitor::ServiceMonitor() : lpfnWTSGetActiveConsoleSessionId("kernel32", "WTSGetActiveConsoleSessionId") {
	GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));
	if (lpfnWTSGetActiveConsoleSessionId.isValid())
		_CurrentSession = (*lpfnWTSGetActiveConsoleSessionId)();
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
		auto lastsession = 0;
		if (lpfnWTSGetActiveConsoleSessionId.isValid())
			lastsession = (*lpfnWTSGetActiveConsoleSessionId)();
		
		if ((lastsession != 0xFFFFFFFF) && (lastsession > 0) && (lastsession != _CurrentSession)){
			_CurrentSession = lastsession;
			if (ProcessInfo.hProcess != 0) TerminateProcess(ProcessInfo.hProcess, 0);//terminate the old program if running
			StartProcess = true;// set program to restart in hew active session
		}
		if (StartProcess){
			//if (ProcessInfo.hProcess != 0) TerminateProcess(ProcessInfo.hProcess, 0);
			memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
			memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));
			StartUPInfo.lpDesktop = L"Winsta0\\default";
			bool process_creation_successful = false;
			StartUPInfo.cb = sizeof(STARTUPINFO);
			HANDLE winloginhandle = NULL;
			if (GetWinlogonHandle(&winloginhandle)){
				SetLastError(0);
				process_creation_successful = CreateProcessAsUser(winloginhandle, NULL, szPath, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &StartUPInfo, &ProcessInfo) == TRUE;
			}
			CloseHandle(winloginhandle);
			StartProcess = false;
			std::ofstream myfile("c:\\example.txt", std::ios::app);
			DWORD error = GetLastError();
			if (!process_creation_successful){
				
				myfile << "process creation failed with error " << error << std::endl;
				//try another method
				
				if (error ==233) CreateRemoteSessionProcess(_CurrentSession, true, winloginhandle, NULL, szPath, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
			}
			else myfile << "process creation successfull " << error << std::endl;


		}

	}
}
