#include "stdafx.h"
#include "ServiceInstaller.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"

//   FUNCTION: InstallService
//
//   PURPOSE: Install the current application as a service to the local 
//   service control manager database.
//
//   PARAMETERS:
//   * pszServiceName - the name of the service to be installed
//   * pszDisplayName - the display name of the service
//   * dwStartType - the service start option. This parameter can be one of 
//     the following values: SERVICE_AUTO_START, SERVICE_BOOT_START, 
//     SERVICE_DEMAND_START, SERVICE_DISABLED, SERVICE_SYSTEM_START.
//   * pszDependencies - a pointer to a double null-terminated array of null-
//     separated names of services or load ordering groups that the system 
//     must start before this service.
//   * pszAccount - the name of the account under which the service runs.
//   * pszPassword - the password to the account name.
//
//   NOTE: If the function fails to install the service, it prints the error 
//   in the standard output stream for users to diagnose the problem.
//
bool InstallService(PWSTR pszServiceName,
	PWSTR pszDisplayName,
	DWORD dwStartType,
	PWSTR pszDependencies,
	PWSTR pszAccount,
	PWSTR pszPassword)
{
	wchar_t szPath[MAX_PATH];
	bool ret = false;
	if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0) {
		wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
		return false;
	}
	auto schSCManager=RAIISC_HANDLE(OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE));
	if (schSCManager.get() == nullptr) wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
	else {
		wcscat_s(szPath, L" -service_mon");
		// Install the service into SCM by calling CreateService
		auto schService =RAIISC_HANDLE(CreateService(
			schSCManager.get(),                   // SCManager database
			pszServiceName,                 // Name of service
			pszDisplayName,                 // Name to display
			SERVICE_ALL_ACCESS,           // Desired access
			SERVICE_WIN32_OWN_PROCESS,      // Service type
			dwStartType,                    // Service start type
			SERVICE_ERROR_NORMAL,           // Error control type
			szPath,                         // Service's binary
			NULL,                           // No load ordering group
			NULL,                           // No tag identifier
			pszDependencies,                // Dependencies
			pszAccount,                     // Service running account
			pszPassword                     // Password of the account
			));
		if (schService.get() == nullptr) DEBUG_MSG("CreateService failed w / err %", GetLastError());
		else {
			wprintf(L"%s is installed.\n", pszServiceName);
			return StartService(schService.get(), 0, NULL);
		}
	}
	return false;
}


//
//   FUNCTION: UninstallService
//
//   PURPOSE: Stop and remove the service from the local service control 
//   manager database.
//
//   PARAMETERS: 
//   * pszServiceName - the name of the service to be removed.
//
//   NOTE: If the function fails to uninstall the service, it prints the 
//   error in the standard output stream for users to diagnose the problem.
//
void UninstallService(PWSTR pszServiceName)
{

	SERVICE_STATUS ssSvcStatus = {};

	// Open the local default service control manager database
	auto schSCManager =RAIISC_HANDLE(OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT));
	if (schSCManager.get() == nullptr) wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
	else {

		// Open the service with delete, stop, and query status permissions
		auto schService = RAIISC_HANDLE(OpenService(schSCManager.get(), pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
		if (schService.get() == nullptr) wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
		else {

			// Try to stop the service
			if (ControlService(schService.get(), SERVICE_CONTROL_STOP, &ssSvcStatus))
			{
				wprintf(L"Stopping %s.", pszServiceName);
				Sleep(1000);
				while (QueryServiceStatus(schService.get(), &ssSvcStatus))
				{
					if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
					{
						wprintf(L".");
						Sleep(1000);
					}
					else break;
				}
				if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) wprintf(L"\n%s is stopped.\n", pszServiceName);
				else wprintf(L"\n%s failed to stop.\n", pszServiceName);
			}
			// Now remove the service by calling DeleteService.
			if (!DeleteService(schService.get())) wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
			wprintf(L"%s is removed.\n", pszServiceName);
		}
	}
}