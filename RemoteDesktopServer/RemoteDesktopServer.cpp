// RemoteDesktopServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ServiceInstaller.h"
#include "ServerService.h"

// Internal name of the service
#define SERVICE_NAME             L"CppWindowsService"
// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"CppWindowsService Sample Service"
// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START
// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""
// The name of the account under which the service should run   NULL is the local system account
#define SERVICE_ACCOUNT          NULL
// The password to the service account name
#define SERVICE_PASSWORD         NULL


int _tmain(int argc, _TCHAR* argv[])
{
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"install", argv[1] + 1) == 0)
		{
			// Install the service when the command is 
			// "-install" or "/install".
			InstallService(
				SERVICE_NAME,               // Name of service
				SERVICE_DISPLAY_NAME,       // Name to display
				SERVICE_START_TYPE,         // Service start type
				SERVICE_DEPENDENCIES,       // Dependencies
				SERVICE_ACCOUNT,            // Service running account
				SERVICE_PASSWORD            // Password of the account
				);
		}
		else if (_wcsicmp(L"remove", argv[1] + 1) == 0)
		{
			// Uninstall the service when the command is 
			// "-remove" or "/remove".
			UninstallService(SERVICE_NAME);
		}
	}
	else
	{


		/*auto serviceMode = false;
		auto SessionID = -1;
		DWORD Size = 0;
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			wprintf(L"OpenProcessToken error 0x%08lx\n", GetLastError());

		if (!GetTokenInformation(hToken, TokenSessionId, &SessionID, sizeof(SessionID), &Size) || !Size)
			return 0;
		if (SessionID == 0)
			serviceMode = true;
		CloseHandle(hToken);*/

		wprintf(L"Parameters:\n");
		wprintf(L" -install  to install the service.\n");
		wprintf(L" -remove   to remove the service.\n");

		ServerService service(SERVICE_NAME);
	//	if (serviceMode){
			if (!ServerService::Run(service))
				wprintf(L"Service failed to run w/error 0x%08lx\n", GetLastError());
		/*}
		else {
			service.OnStart(0, 0);
		}
*/
		wprintf(L"waiting on input\n");
		service.Stop();
	}
	return 0;
}

