#include "stdafx.h"
#include "ServiceInstaller.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\EventLog.h"

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

	UninstallService(pszServiceName);
	//std::ofstream file("c:\\users\\slee\\desktop\\text.txt", std::ios::app);
	wchar_t szPath[MAX_PATH];
	bool ret = false;
	if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0) {
		//file << "GetModuleFileName failed w/err " << GetLastError() << std::endl;
		//wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
		return false;
	}

	std::wstring tmp(szPath);
	tmp = L"\"" + tmp + L"\"";
	wcsncpy_s(szPath, tmp.c_str(), tmp.size()+1);

	auto schSCManager = RAIISC_HANDLE(OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE));
	if (schSCManager.get() == nullptr) {
		//file << "OpenSCManager failed w/err " << GetLastError() << std::endl;
		//wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
	}
	else {
		wcscat_s(szPath, L" -service_mon");
		// Install the service into SCM by calling CreateService
		auto schService = RAIISC_HANDLE(CreateService(
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
		if (schService.get() == nullptr){
	/*		file << "CreateService failed w/err " << GetLastError() << std::endl;
			DEBUG_MSG("CreateService failed w / err %", GetLastError());*/
			return false;
		}
		else {
		//	RemoteDesktop::EventLog::WriteLog(L"Service Installed " + std::wstring(pszServiceName), RemoteDesktop::EventLog::EventType::INFORMATIONAL, RemoteDesktop::EventLog::EventCategory::NETWORK_CATEGORY, RemoteDesktop::EventLog::EventID::SERVICE);
		//	file << "Service is installed" << std::endl;
			//wprintf(L"%s is installed.\n", pszServiceName);

			SERVICE_STATUS_PROCESS ssStatus;
			DWORD dwOldCheckPoint;
			DWORD dwStartTickCount;
			DWORD dwWaitTime;
			DWORD dwBytesNeeded;

			// Check the status in case the service is not stopped. 

			if (!QueryServiceStatusEx(
				schService.get(),                     // handle to service 
				SC_STATUS_PROCESS_INFO,         // information level
				(LPBYTE)&ssStatus,             // address of structure
				sizeof(SERVICE_STATUS_PROCESS), // size of structure
				&dwBytesNeeded))              // size needed if buffer is too small
			{
				//printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
				return false;
			}

			// Check if the service is already running. It would be possible 
			// to stop the service here, but for simplicity this example just returns. 

			if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
			{
				//printf("Cannot start the service because it is already running\n");
				return false;
			}

			// Save the tick count and initial checkpoint.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;

			// Wait for the service to stop before attempting to start it.

			while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				// Do not wait longer than the wait hint. A good interval is 
				// one-tenth of the wait hint but not less than 1 second  
				// and not more than 10 seconds. 

				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 1000)
					dwWaitTime = 1000;
				else if (dwWaitTime > 10000)
					dwWaitTime = 10000;

				Sleep(dwWaitTime);

				// Check the status until the service is no longer stop pending. 

				if (!QueryServiceStatusEx(
					schService.get(),                     // handle to service 
					SC_STATUS_PROCESS_INFO,         // information level
					(LPBYTE)&ssStatus,             // address of structure
					sizeof(SERVICE_STATUS_PROCESS), // size of structure
					&dwBytesNeeded))              // size needed if buffer is too small
				{
					//printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
					return false;
				}

				if (ssStatus.dwCheckPoint > dwOldCheckPoint)
				{
					// Continue to wait and check.

					dwStartTickCount = GetTickCount();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else
				{
					if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
					{
						//printf("Timeout waiting for service to stop\n");
						return false;
					}
				}
			}

			// Attempt to start the service.

			if (!StartService( schService.get(),  0, NULL)) 
			{
				//printf("StartService failed (%d)\n", GetLastError());
				return false;
			}
		//	else printf("Service start pending...\n");

			// Check the status until the service is no longer start pending. 

			if (!QueryServiceStatusEx(
				schService.get(),                     // handle to service 
				SC_STATUS_PROCESS_INFO,         // info level
				(LPBYTE)&ssStatus,             // address of structure
				sizeof(SERVICE_STATUS_PROCESS), // size of structure
				&dwBytesNeeded))              // if buffer too small
			{
				//printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
				return false;
			}

			// Save the tick count and initial checkpoint.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;

			while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
			{
				// Do not wait longer than the wait hint. A good interval is 
				// one-tenth the wait hint, but no less than 1 second and no 
				// more than 10 seconds. 

				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 1000)
					dwWaitTime = 1000;
				else if (dwWaitTime > 10000)
					dwWaitTime = 10000;

				Sleep(dwWaitTime);

				// Check the status again. 

				if (!QueryServiceStatusEx(
					schService.get(),             // handle to service 
					SC_STATUS_PROCESS_INFO, // info level
					(LPBYTE)&ssStatus,             // address of structure
					sizeof(SERVICE_STATUS_PROCESS), // size of structure
					&dwBytesNeeded))              // if buffer too small
				{
					//printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
					break;
				}

				if (ssStatus.dwCheckPoint > dwOldCheckPoint)
				{
					// Continue to wait and check.

					dwStartTickCount = GetTickCount();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else
				{
					if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
					{
						// No progress made within the wait hint.
						break;
					}
				}
			}

			// Determine whether the service is running.

			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				//printf("Service started successfully.\n");
				return true;
			}
			else
			{
		/*		printf("Service not started. \n");
				printf("  Current State: %d\n", ssStatus.dwCurrentState);
				printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
				printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
				printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);*/
				return false;
			}
			//return StartService(schService.get(), 0, NULL) == TRUE;
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
	//std::ofstream file("c:\\users\\slee\\desktop\\text.txt", std::ios::app);
	SERVICE_STATUS ssSvcStatus = {};
	// Open the local default service control manager database
	auto schSCManager = RAIISC_HANDLE(OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT));
	if (schSCManager.get() == nullptr) {
		//file << "OpenSCManager failed w/err" << GetLastError()<<std::endl;
		//wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
	}
	else {

		// Open the service with delete, stop, and query status permissions
		auto schService = RAIISC_HANDLE(OpenService(schSCManager.get(), pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
		if (schService.get() == nullptr) {
	/*		file << "OpenService failed w/err" << GetLastError() << std::endl;
			wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());*/
		}
		else {

			// Try to stop the service
			if (ControlService(schService.get(), SERVICE_CONTROL_STOP, &ssSvcStatus))
			{
			/*	file << "Stopping"  << std::endl;
				wprintf(L"Stopping %s.", pszServiceName);*/
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
				if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) {
				/*	file << "Stopped" << std::endl;
					wprintf(L"\n%s is stopped.\n", pszServiceName);*/
				}
				else{
					/*file << "Failed to stop" << std::endl;
					wprintf(L"\n%s failed to stop.\n", pszServiceName);*/
				}
			}
			// Now remove the service by calling DeleteService.
			if (!DeleteService(schService.get())) {
				/*file << "DeleteService failed w/err" << GetLastError() << std::endl;
				wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());*/
			}
			else {
				//RemoteDesktop::EventLog::WriteLog(L"Service uninstalled " + std::wstring(pszServiceName), RemoteDesktop::EventLog::EventType::INFORMATIONAL, RemoteDesktop::EventLog::EventCategory::NETWORK_CATEGORY, RemoteDesktop::EventLog::EventID::SERVICE);
			//	file << "Service uninstalled" << std::endl;
				Sleep(3000);//pause here on success
			}
		//	wprintf(L"%s is removed.\n", pszServiceName);
		}
	}
}