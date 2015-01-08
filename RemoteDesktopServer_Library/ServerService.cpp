#include "stdafx.h"
#include "ServerService.h"
#include "ServiceMonitor.h"
#include "..\RemoteDesktop_Library\EventLog.h"

ServerService::ServerService(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	
}
ServerService::~ServerService(void)
{

}
void ServerService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
	// Log a service start message to the Application log.
	RemoteDesktop::EventLog::WriteLog(L"CppWindowsService in OnStart", EVENTLOG_INFORMATION_TYPE);

	_ServiceMonitor = std::make_unique<RemoteDesktop::ServiceMonitor>();
	_ServiceMonitor->Start();
}
void ServerService::OnStop()
{
	// Log a service stop message to the Application log.
	RemoteDesktop::EventLog::WriteLog(L"CppWindowsService in OnStop", EVENTLOG_INFORMATION_TYPE);
	_ServiceMonitor->Stop();
	_ServiceMonitor = nullptr;
}
void ServerService::OnSessionChange(DWORD eventtype){

}
void  ServerService::OnShutdown(){
	OnStop();
}