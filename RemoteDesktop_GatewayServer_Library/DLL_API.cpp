#include "stdafx.h"
#include "DLL_API.h"
#include "..\RemoteDesktop_Library\Network_GatewayServer.h"

void* __stdcall Create_Server(void(__stdcall * onconnect)(),
	void(__stdcall * ondisconnect)()){
	return new RemoteDesktop::GatewayServer(onconnect, ondisconnect);
}

void __stdcall Destroy_Server(void* server){
	if (server == nullptr) return;
	auto ptr = (RemoteDesktop::GatewayServer*)server;
	delete ptr;
}

void __stdcall Listen(void* server, wchar_t* ip_or_host, wchar_t* port){
	if (server == nullptr) return;
	auto ptr = (RemoteDesktop::GatewayServer*)server;
	ptr->Start(ip_or_host, port);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

