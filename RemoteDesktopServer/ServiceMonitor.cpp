#include "stdafx.h"
#include "ServiceMonitor.h"
#include "ServiceHelpers.h"
#include "Userenv.h"
#include <fstream>
#include "..\RemoteDesktop_Library\Event_Wrapper.h"

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

	Event_Wrapper exitprog(CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEventRDProgram"));
	Event_Wrapper cardreq(CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEvenRDCad"));

	while (Running){
		DEBUG_MSG("Waiting for CAD Event");
		auto Index = WaitForSingleObject(cardreq.get_Handle(), 1000);
		if (Index == 0){
			typedef VOID(WINAPI *SendSas)(BOOL asUser);
			HINSTANCE Inst = LoadLibrary(L"sas.dll");
			SendSas sendSas = (SendSas)GetProcAddress(Inst, "SendSAS");
			if (sendSas) sendSas(FALSE);
			if (Inst) FreeLibrary(Inst);
		}
		if (lpfnWTSGetActiveConsoleSessionId.isValid())
			_LastSession = (*lpfnWTSGetActiveConsoleSessionId)();

		if ((_LastSession != 0xFFFFFFFF) && (_LastSession >= 0) && (_LastSession != _CurrentSession)){
			_CurrentSession = _LastSession;
			DWORD exitcode = 0;
			SetEvent(exitprog.get_Handle()); // signal to shut down if running
			Sleep(3000);
			if (ProcessInfo.hProcess == 0) {//this is the first launch of the application
				_LaunchProcess();
			}
			else if (GetExitCodeProcess(ProcessInfo.hProcess, &exitcode) != 0)
			{//the program is running and the service will wait for it to exit
				if (exitcode != STILL_ACTIVE)
				{
					Sleep(1000);
					WaitForSingleObject(ProcessInfo.hProcess, 5000);
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
					_LaunchProcess();
				}
				else {
		
					TerminateProcess(ProcessInfo.hProcess, 0);
					Sleep(3000);
					if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
					if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
					_LaunchProcess();
				}
			}
			else {//the application is not running, a new process can be started 
				Sleep(3000);

				if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				_LaunchProcess();
			}
		}

	}
	if (exitprog.get_Handle()) SetEvent(exitprog.get_Handle()); // signal to shut down if running

	if (ProcessInfo.hProcess)
	{
		WaitForSingleObject(ProcessInfo.hProcess, 5000);
		if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
		if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
	}
}

void RemoteDesktop::ServiceMonitor::_LaunchProcess(){

	memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
	memset(&ProcessInfo, 0, sizeof(PROCESS_INFORMATION));

	StartUPInfo.lpDesktop = L"Winsta0\\Winlogon";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	HANDLE winloginhandle = NULL;
	PVOID	lpEnvironment = NULL;

	if (GetWinlogonHandle(&winloginhandle, _CurrentSession)){

		if (CreateEnvironmentBlock(&lpEnvironment, winloginhandle, FALSE) == FALSE) lpEnvironment = NULL;
		SetLastError(0);

		CreateProcessAsUser(winloginhandle, NULL, szPath, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, &ProcessInfo);

	}
	if (lpEnvironment) DestroyEnvironmentBlock(lpEnvironment);
	CloseHandle(winloginhandle);

}