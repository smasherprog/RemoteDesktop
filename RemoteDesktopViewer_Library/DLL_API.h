#ifndef DLL_API123_H
#define DLL_API123_H

#include "..\RemoteDesktop_Library\CommonNetwork.h"

#define DLLEXPORT __declspec( dllexport )  

extern "C" {

	DLLEXPORT void* __stdcall Create_Client(void* hwnd, void(__stdcall * onconnect)(),
		void(__stdcall * ondisconnect)(),
		void(__stdcall * oncursorchange)(int),
		void(__stdcall * ondisplaychanged)(int, int, int, int, int),
		void(__stdcall * onconnectingattempt)(int, int));
	DLLEXPORT void __stdcall Destroy_Client(void* client);
	DLLEXPORT void __stdcall Connect(void* client, wchar_t* port, wchar_t* ip_or_host,int id, wchar_t* aeskey);
	DLLEXPORT void __stdcall Draw(void* client, HDC hdc);
	DLLEXPORT void __stdcall KeyEvent(void* client, int VK, bool down);
	DLLEXPORT void __stdcall MouseEvent(void* client, unsigned int action, int x, int y, int wheel);
	DLLEXPORT void __stdcall SendCAD(void* client);
	DLLEXPORT void __stdcall SendRemoveService(void* client);
	DLLEXPORT void __stdcall ElevateProcess(void* client, wchar_t* username, wchar_t* password);
	DLLEXPORT void __stdcall SendFile(void* client, const char* absolute_path, const char* relative_path, void(__stdcall * onfilechanged)(int));
	DLLEXPORT void __stdcall SendSettings(void* client, int img_quality, bool gray, bool shareclip);
	DLLEXPORT RemoteDesktop::Traffic_Stats __stdcall get_TrafficStats(void* client);
		
	//CALLBACKS
	DLLEXPORT void __stdcall SetOnElevateFailed(void* client, void(__stdcall * func)());
	DLLEXPORT void __stdcall SetOnElevateSuccess(void* client, void(__stdcall * func)());

}

#endif