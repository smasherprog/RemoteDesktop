#ifndef DLL_API123_H
#define DLL_API123_H

#include "..\RemoteDesktop_Library\CommonNetwork.h"

#define DLLEXPORT __declspec( dllexport )  

extern "C" {

	DLLEXPORT void* Create_Client(void* hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int));
	DLLEXPORT void Destroy_Client(void* client);
	DLLEXPORT void Connect(void* client, wchar_t* ip_or_host, wchar_t* port);
	DLLEXPORT void Draw(void* client, HDC hdc);
	DLLEXPORT void KeyEvent(void* client, int VK, bool down);
	DLLEXPORT void MouseEvent(void* client, unsigned int action, int x, int y, int wheel);
	DLLEXPORT void SendCAD(void* client);
	DLLEXPORT void SendFile(void* client, const char* absolute_path, const char* relative_path);
	DLLEXPORT RemoteDesktop::Traffic_Stats get_TrafficStats(void* client);
}

#endif