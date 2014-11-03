#ifndef NETWORKING_H
#define NETWORKING_H
#include "CommonNetwork.h"

#define DECLDIR __declspec(dllexport)

extern "C"
{
	DECLDIR void* CreateServer();
	DECLDIR void DestroyServer(void* server);

	DECLDIR void Listen(void* server, unsigned short port);
	DECLDIR void SetOnConnectCallback(void* server, OnConnectCB onconnect);

	DECLDIR void* CreateClient(HWND hwnd);
	DECLDIR void DestroyClient(void* client);
}

#endif