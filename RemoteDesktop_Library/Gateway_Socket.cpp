#include "stdafx.h"
#include "Gateway_Socket.h"
#include "NetworkSetup.h"

void RemoteDesktop::Gateway_Socket::Receive(){
	ReceiveLoop(_Socket->socket, _Buffer, _BufferCount);

}