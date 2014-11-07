#include "stdafx.h"
#include "ServerService.h"
#include "Server.h"

ServerService::ServerService(PWSTR pszServiceName,
	BOOL fCanStop,
	BOOL fCanShutdown,
	BOOL fCanPauseContinue)
	: CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	
}
ServerService::~ServerService(void)
{
	if (_Server != nullptr) {
		_Server->Stop();
		_Server.release();//this blocks until the process is stopped
	}
}
void ServerService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
	// Log a service start message to the Application log.
	WriteEventLogEntry(L"CppWindowsService in OnStart",
		EVENTLOG_INFORMATION_TYPE);

	_Server = std::make_unique<RemoteDesktop::Server>();
	_Server->Listen(443);
}
void ServerService::OnStop()
{
	// Log a service stop message to the Application log.
	WriteEventLogEntry(L"CppWindowsService in OnStop",
		EVENTLOG_INFORMATION_TYPE);
	if (_Server != nullptr) {
		_Server->Stop();	
		_Server.release();//this blocks until the process is stopped
	}
}