#include "stdafx.h"
#include "Networking.h"
#include "Server.h"
#include "Client.h"

void* CreateServer(){
	return new RemoteDesktop::Server();
}
void Listen(void* server, unsigned short port){
	if (server == NULL) return;
	auto s = (RemoteDesktop::Server*)server;
	s->Listen(port);
}
void DestroyServer(void* server){
	if (server != NULL) delete server;
}	
void SetOnConnectCallback(void* server, OnConnectCB callback){
	if (server != NULL) {
		auto s = (RemoteDesktop::IServer*)server;
		s->SetOnConnectCallback(callback);
	}
}
void* CreateClient(){
	return new RemoteDesktop::Client();

}
void DestroyClient(void* client){
	if (client != NULL) delete client;
}