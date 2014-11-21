#ifndef DLL_API123_H
#define DLL_API123_H


#define DLLEXPORT __declspec( dllexport )  

extern "C" {

	DLLEXPORT void* Create_Client(void* hwnd);
	DLLEXPORT void Destroy_Client(void* client);
	DLLEXPORT void Connect(void* client, wchar_t* ip_or_host, wchar_t* port);
	DLLEXPORT void Draw(void* client, HDC hdc);
	DLLEXPORT void KeyEvent(void* client, int VK, bool down);
	DLLEXPORT void MouseEvent(void* client, unsigned int action, int x, int y, int wheel);
	DLLEXPORT void SendCAD(void* client);

}

#endif