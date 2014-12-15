// RemoteDesktopServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\RemoteDesktop_Library\Config.h"
#include "..\RemoteDesktopServer_Library\ServiceInstaller.h"
#include "..\RemoteDesktopServer_Library\ServerService.h"
#include "..\RemoteDesktopServer_Library\RD_Server.h"

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int argc;
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"uninstall", argv[1] + 1) == 0)
		{
			// Uninstall the service when the command is 
			// "-remove" or "/remove".
			UninstallService(SERVICE_NAME);
		}
		else if (_wcsicmp(L"delayed_uninstall", argv[1] + 1) == 0)
		{
			Sleep(3000);//give enough time for the previous programs to shut down
			UninstallService(SERVICE_NAME);
		}
		else if (_wcsicmp(L"service_mon", argv[1] + 1) == 0)
		{
			ServerService service(SERVICE_NAME);
			ServerService::Run(service);
		}
		else if (_wcsicmp(L"run", argv[1] + 1) == 0)
		{
			auto _Server = std::make_unique<RemoteDesktop::RD_Server>();
			_Server->Listen(DEFAULTPORT);
		}
	}
	else
	{
		//try to install the service if launched normally. If the service cannot be installed it is because its already installed, or the progrm was not launched with correct permissions.
		//in which case, try to launch the program normally.
		if (!InstallService(
			SERVICE_NAME,               // Name of service
			SERVICE_DISPLAY_NAME,       // Name to display
			SERVICE_AUTO_START,
			L"",
			NULL,
			NULL
			)){
			auto _Server = std::make_unique<RemoteDesktop::RD_Server>();
			_Server->Listen(DEFAULTPORT);
		}
	}

	return 0;
}

