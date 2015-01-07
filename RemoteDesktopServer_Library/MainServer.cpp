#include "stdafx.h"
#include "MainServer.h"
#include "Config.h"
#include "ServiceInstaller.h"
#include "ServerService.h"
#include "RD_Server.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"

void RemoteDesktop::Startup(LPWSTR* argv, int argc, bool startasproxy){

#if !_DEBUG
	if (RemoteDesktop::TryToElevate(argv, argc)) return;//if the app was able to elevate, shut this instance down
#endif
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"uninstall", argv[1] + 1) == 0) UninstallService(Service_Name());
		else if (_wcsicmp(L"delayed_uninstall", argv[1] + 1) == 0)
		{
			Sleep(3000);//give enough time for the previous programs to shut down
			UninstallService(Service_Name());
		}
		else if (_wcsicmp(L"service_mon", argv[1] + 1) == 0)
		{
			ServerService service(Service_Name());
			ServerService::Run(service);
		}
		else if (_wcsicmp(L"run", argv[1] + 1) == 0)
		{
			auto _Server = std::make_unique<RemoteDesktop::RD_Server>();
			if (startasproxy) _Server->Listen(DefaultPort(), DefaultGateway(), startasproxy);
			else _Server->Listen(DefaultPort());
		}
	}
	else
	{
		
		auto ret = IDYES;
		if (startasproxy){
			ret = MessageBox(
				NULL,
				DisclaimerMessage(),
				(LPCWSTR)L"Disclaimer",
				MB_ICONQUESTION | MB_YESNO
				);
		}
		//try to install the service if launched normally. If the service cannot be installed it is because its already installed, or the progrm was not launched with correct permissions.
		//in which case, try to launch the program normally.
		if (ret == IDYES){
			//the below line was added so that in cases where UAC is required to access the network, it will activate the pop up as early as possible
			if (!InstallService(
				Service_Name(),               // Name of service
				Service_Display_Name(),       // Name to display
				SERVICE_AUTO_START,
				L"",
				NULL,
				NULL
				)){
				auto _Server = std::make_unique<RemoteDesktop::RD_Server>();
				if (startasproxy) _Server->Listen(DefaultPort(), DefaultGateway(), startasproxy);
				else _Server->Listen(DefaultPort());
			}
		}
	}
}


