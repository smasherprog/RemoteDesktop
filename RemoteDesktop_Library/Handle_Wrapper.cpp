#include "stdafx.h"
#include "Handle_Wrapper.h"

void RemoteDesktop::INTERNAL::dialogboxcleanup(HWND h){
	if (h!=nullptr) EndDialog(h, 0); 
}
void RemoteDesktop::INTERNAL::hwndtimercleanup(HWNDTimer* h){
	if (h != nullptr){
		KillTimer(h->h, h->id);
		delete h;
	}
}
void RemoteDesktop::INTERNAL::socketcleanup(SOCKETWrapper* s){
	if (s->socket != INVALID_SOCKET){
		shutdown(s->socket, SD_SEND);//allow sends to go out.. stop receiving
		closesocket(s->socket);
	}
	delete s;
}
