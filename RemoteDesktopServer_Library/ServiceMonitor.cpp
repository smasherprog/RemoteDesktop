#include "stdafx.h"
#include "ServiceMonitor.h"
#include "ServiceHelpers.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "SoftwareCAD.h"
#include "sas.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"


RemoteDesktop::ServiceMonitor::ServiceMonitor(){
	if (!IsSASCadEnabled()) Enable_SASCAD();
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
	BEGINTRY
		if (std::this_thread::get_id() != _BackGroundNetworkWorker.get_id() && _BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	ENDTRY
}

void RemoteDesktop::ServiceMonitor::_Run(){
	//wait for any existing program to stop running
	HANDLE hEvent = NULL;
	while ((hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\SessionEventRDProgram")) != NULL) {
		SetEvent(hEvent); // signal to shut down if running
		CloseHandle(hEvent);
		Sleep(1000);
	}

	//make sure to enable software cad
	auto ExitProgHandle(RAIIHANDLE(CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEventRDProgram")));
	auto cardreq(RAIIHANDLE(CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEventRDCad")));
	auto selfremovaltrigger(RAIIHANDLE(CreateEvent(NULL, FALSE, FALSE, L"Global\\SessionEventRemoveSelf")));

	HANDLE evs[2];
	evs[0] = cardreq.get();
	evs[1] = selfremovaltrigger.get();

	while (Running){
		DEBUG_MSG("Waiting for CAD Event");

		auto Index = WaitForMultipleObjects(2, evs, false, 1000);
		if (Index == 0){
			SendSAS(FALSE);
		}
		else if (Index == 1){
			Running = false;
			_LaunchProcess(L" -delayed_uninstall");
			Cleanup_System_Configuration();
			break;
		}

		_LastSession = WTSGetActiveConsoleSessionId();

		if ((_LastSession != 0xFFFFFFFF) && (_LastSession >= 0) && (_LastSession != _CurrentSession)){
			_CurrentSession = _LastSession;
			DWORD exitcode = 0;
			SetEvent(ExitProgHandle.get()); // signal to shut down if running
			Sleep(3000);
			if (!_App){//this is the first launch of the application
				_App = _LaunchProcess(L" -run");
			}
			else if (GetExitCodeProcess(_App->hProcess, &exitcode) != 0)
			{//the program is running and the service will wait for it to exit
				if (exitcode != STILL_ACTIVE)
				{
					Sleep(1000);
					WaitForSingleObject(_App->hProcess, 5000);
					_App = _LaunchProcess(L" -run");
				}
				else {

					TerminateProcess(_App->hProcess, 0);
					Sleep(3000);
					_App = _LaunchProcess(L" -run");
				}
			}
			else {//the application is not running, a new process can be started 
				Sleep(3000);
				_App = _LaunchProcess(L" -run");
			}
		}

	}
	if (ExitProgHandle.get() != nullptr) SetEvent(ExitProgHandle.get()); // signal to shut down if running
	if (_App) WaitForSingleObject(_App->hProcess, 5000);

}

std::shared_ptr<PROCESS_INFORMATION> RemoteDesktop::ServiceMonitor::_LaunchProcess(wchar_t* args){
	wchar_t szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
	std::wstring tmp(szPath);
	tmp = L"\"" + tmp + L"\"";
	wcsncpy_s(szPath, tmp.c_str(), tmp.size()+1);
	if (args != nullptr) wcscat_s(szPath, args);
	return LaunchProcess(szPath, _CurrentSession);
}
