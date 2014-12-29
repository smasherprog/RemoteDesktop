#include "stdafx.h"
#include "MainServer.h"
#include "..\RemoteDesktop_Library\Config.h"
#include "ServiceInstaller.h"
#include "ServerService.h"
#include "RD_Server.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"

void RemoteDesktop::Startup(LPWSTR* argv, int argc, bool startasproxy){

	
	if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
	{
		if (_wcsicmp(L"uninstall", argv[1] + 1) == 0) UninstallService(SERVICE_NAME);
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
	
			if (startasproxy) _Server->Listen(DEFAULTPORT, DEFAULTPROXY, startasproxy);
			else _Server->Listen(DEFAULTPORT);
		}
	}
	else
	{

		auto ret = IDYES;
		if (startasproxy){
			ret = MessageBox(
				NULL,
				(LPCWSTR)L"Do you agree to allow a support technician to connection to your computer?",
				(LPCWSTR)L"Disclaimer",
				MB_ICONQUESTION | MB_YESNO
				);
		}

		//try to install the service if launched normally. If the service cannot be installed it is because its already installed, or the progrm was not launched with correct permissions.
		//in which case, try to launch the program normally.
		if (ret == IDYES){
			//the below line was added so that in cases where UAC is required to access the network, it will activate the pop up as early as possible
			RemoteDesktop::PrimeNetwork(DEFAULTPORT);
			if (!InstallService(
				SERVICE_NAME,               // Name of service
				SERVICE_DISPLAY_NAME,       // Name to display
				SERVICE_AUTO_START,
				L"",
				NULL,
				NULL
				)){
				auto _Server = std::make_unique<RemoteDesktop::RD_Server>();
				if (startasproxy) _Server->Listen(DEFAULTPORT, DEFAULTPROXY, startasproxy);
				else _Server->Listen(DEFAULTPORT);
			}
		}
	}
}


