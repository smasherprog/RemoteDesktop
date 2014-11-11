#include "stdafx.h"
#include "ServiceMonitor.h"
#include "ServiceHelpers.h"
#include "Userenv.h"
#include <fstream>
RemoteDesktop::ServiceMonitor::ServiceMonitor() : lpfnWTSGetActiveConsoleSessionId("kernel32", "WTSGetActiveConsoleSessionId") {
	GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
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

}
void wait_for_existing_process()
{
	HANDLE hEvent = NULL;
	while ((hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\SessionEventRDProgram")) != NULL) {
		SetEvent(hEvent); // signal to shut down if running
		CloseHandle(hEvent);
		Sleep(1000);
	}
}
void RemoteDesktop::ServiceMonitor::_Run(){

	wait_for_existing_process();//wait for any existing program to stop running

	auto exitrdprogram = CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEventRDProgram");


	while (Running){

		Sleep(3000);
		if (lpfnWTSGetActiveConsoleSessionId.isValid())
			_LastSession = (*lpfnWTSGetActiveConsoleSessionId)();
		std::ofstream myfile("c:\\example.txt", std::ios::app);
		//myfile << "session " << _LastSession << "   " << _CurrentSession << std::endl;

		if ((_LastSession != 0xFFFFFFFF) && (_LastSession >= 0) && (_LastSession != _CurrentSession)){
			_CurrentSession = _LastSession;
			DWORD exitcode = 0;
		//	myfile << "changed needed " << std::endl;
			SetEvent(exitrdprogram); // signal to shut down if running
			if (ProcessInfo.hProcess == 0) {//this is the first launch of the application
				_LaunchProcess(myfile);
				//myfile << "first " << std::endl;
			}
			else if (GetExitCodeProcess(ProcessInfo.hProcess, &exitcode) != 0)
			{//the program is running and the service will wait for it to exit
				if (exitcode != STILL_ACTIVE)
				{
					//myfile << "STILL_ACTIVE " << std::endl;
					Sleep(1000);
					WaitForSingleObject(ProcessInfo.hProcess, 5000);
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
					_LaunchProcess(myfile);
				}
				else {
					//myfile << "NOT STILL_ACTIVE " << std::endl; 
					TerminateProcess(ProcessInfo.hProcess, 0);
					Sleep(3000);
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
					_LaunchProcess(myfile);
				}
			}
			else {//the application is not running, a new process can be started 
				Sleep(3000);
				//myfile << " else GetExitCodeProcess " << std::endl;
				
				if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				_LaunchProcess(myfile);
			}
		}

	}
	if (exitrdprogram) SetEvent(exitrdprogram); // signal to shut down if running

	if (ProcessInfo.hProcess)
	{
		WaitForSingleObject(ProcessInfo.hProcess, 5000);
		if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
		if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
	}

	if (exitrdprogram) CloseHandle(exitrdprogram);//close handle

}

void RemoteDesktop::ServiceMonitor::_LaunchProcess(std::ofstream& myfile){

	memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
	memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));

	StartUPInfo.lpDesktop = L"Winsta0\\Winlogon";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	HANDLE winloginhandle = NULL;
	PVOID	lpEnvironment = NULL;

	if (GetWinlogonHandle(&winloginhandle, _CurrentSession)){
		//myfile << "GetWinlogonHandle" << std::endl;
		if (CreateEnvironmentBlock(&lpEnvironment, winloginhandle, FALSE) == FALSE) lpEnvironment = NULL;
		SetLastError(0);
		//myfile << "CreateEnvironmentBlock" << lpEnvironment << std::endl;
		if (CreateProcessAsUser(winloginhandle, NULL, szPath, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo) == FALSE){
			DWORD error = GetLastError();
		//	myfile << "CreateProcessAsUser error" << error << std::endl;
			//if (error == 233) CreateRemoteSessionProcess(_CurrentSession, true, winloginhandle, NULL, szPath, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &StartUPInfo, &ProcessInfo);
		}
		else {
			DWORD error1 = GetLastError();
		//	myfile << "CreateProcessAsUser success" << error1 << std::endl;
		}



	}
	if (lpEnvironment) DestroyEnvironmentBlock(lpEnvironment);
	CloseHandle(winloginhandle);

}