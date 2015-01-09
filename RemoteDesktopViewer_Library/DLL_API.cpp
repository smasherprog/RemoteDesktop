#include "stdafx.h"
#include "DLL_API.h"
#include "Client.h"

void* __stdcall Create_Client(void* hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int), void(__stdcall * ondisplaychanged)(int, int, int, int, int), void(__stdcall * onconnectingattempt)(int, int)){
	return new RemoteDesktop::Client((HWND)hwnd, onconnect, ondisconnect, oncursorchange, ondisplaychanged, onconnectingattempt);
}
void __stdcall Destroy_Client(void* client){
	if (client != NULL) {
		auto c = (RemoteDesktop::Client*)client; 
		delete c;
	}
}
void __stdcall Connect(void* client, wchar_t* ip_or_host, wchar_t* port, int id, wchar_t* aeskey){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->Connect(ip_or_host, port, id, aeskey);
}
void __stdcall Draw(void* client, HDC hdc){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->Draw(hdc);
}

void __stdcall KeyEvent(void* client, int VK, bool down){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->KeyEvent(VK,down);
}
void __stdcall MouseEvent(void* client, unsigned int action, int x, int y, int wheel){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->MouseEvent(action, x, y, wheel);
}
void __stdcall SendCAD(void* client){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->SendCAD();
}
void __stdcall SendRemoveService(void* client){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->SendRemoveService();
}

void __stdcall SendFile(void* client, const char* absolute_path, const char* relative_path, void(__stdcall * onfilechanged)(int)){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	c->SendFile(absolute_path, relative_path, onfilechanged);
}
void __stdcall SendSettings(void* client,  int img_quality, bool gray, bool shareclip){
	if (client == NULL)return;
	auto c = (RemoteDesktop::Client*)client;
	RemoteDesktop::Settings_Header h;
	h.GrayScale = gray;
	h.Image_Quality = img_quality;
	h.ShareClip = shareclip;
	c->SendSettings(h);
}


RemoteDesktop::Traffic_Stats __stdcall get_TrafficStats(void* client){
	if (client == NULL){
		RemoteDesktop::Traffic_Stats tmp;
		memset(&tmp, 0, sizeof(tmp));
		return tmp;
	}
	auto c = (RemoteDesktop::Client*)client;
	auto s= c->get_TrafficStats();
	return s;
}