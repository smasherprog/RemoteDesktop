#include "stdafx.h"
#include "DLL_API.h"
#include "Client.h"

void* Create_Client(void* hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int)){
	return new RemoteDesktop::Client((HWND)hwnd, onconnect, ondisconnect, oncursorchange);
}
void Destroy_Client(void* client){
	if (client != NULL) {
		auto c = (RemoteDesktop::Client*)client; 
		delete c;
	}
}
void Connect(void* client, wchar_t* ip_or_host, wchar_t* port){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->Connect(ip_or_host, port);
}
void Draw(void* client, HDC hdc){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->Draw(hdc);
}

void KeyEvent(void* client, int VK, bool down){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->KeyEvent(VK,down);
}
void MouseEvent(void* client, unsigned int action, int x, int y, int wheel){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->MouseEvent(action, x, y, wheel);
}
void SendCAD(void* client){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->SendCAD();
}
void SendFile(void* client, const char* absolute_path, const char* relative_path){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->SendFile(absolute_path, relative_path);
}
RemoteDesktop::Traffic_Stats get_TrafficStats(void* client){
	if (client == NULL){
		RemoteDesktop::Traffic_Stats tmp;
		memset(&tmp, 0, sizeof(tmp));
		return tmp;
	}
	auto c = (RemoteDesktop::Client*)client;
	auto s= c->get_TrafficStats();
	return s;
}