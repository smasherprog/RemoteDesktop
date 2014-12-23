#ifndef DLL_API123_H
#define DLL_API123_H

#include "..\RemoteDesktop_Library\CommonNetwork.h"

#define DLLEXPORT __declspec( dllexport )  

extern "C" {

	DLLEXPORT void* __stdcall Create_Client(void* hwnd, void(__stdcall * onconnect)(),
		void(__stdcall * ondisconnect)(),
		void(__stdcall * oncursorchange)(int),
		void(__stdcall * onprimchanged)(int, int),
		void(__stdcall * onconnectingattempt)(int, int));
	DLLEXPORT void __stdcall Destroy_Client(void* client);
	DLLEXPORT void __stdcall Connect(void* client, wchar_t* ip_or_host, wchar_t* port, int id, wchar_t* aeskey);
	DLLEXPORT void __stdcall Draw(void* client, HDC hdc);
	DLLEXPORT void __stdcall KeyEvent(void* client, int VK, bool down);
	DLLEXPORT void __stdcall MouseEvent(void* client, unsigned int action, int x, int y, int wheel);
	DLLEXPORT void __stdcall SendCAD(void* client);
	DLLEXPORT void __stdcall SendRemoveService(void* client);
	DLLEXPORT void __stdcall SendFile(void* client, const char* absolute_path, const char* relative_path);
	DLLEXPORT void __stdcall SendImageSettings(void* client, int quality, bool grayascale);
	
	DLLEXPORT RemoteDesktop::Traffic_Stats __stdcall get_TrafficStats(void* client);
}

#endif