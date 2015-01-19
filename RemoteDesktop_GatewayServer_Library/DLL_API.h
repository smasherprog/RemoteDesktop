#ifndef DLL_API123_H
#define DLL_API123_H

#include "..\RemoteDesktop_Library\CommonNetwork.h"

#define DLLEXPORT __declspec( dllexport )  

extern "C" {

	DLLEXPORT void* __stdcall Create_Server(void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)());
	DLLEXPORT void __stdcall Destroy_Server(void* server);
	DLLEXPORT void __stdcall Listen(void* server, wchar_t* ip_or_host, wchar_t* port);

}

#endif