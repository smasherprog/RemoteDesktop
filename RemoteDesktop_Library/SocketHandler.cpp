#include "stdafx.h"
#include "SocketHandler.h"
#include "Encryption.h"

RemoteDesktop::socket_wrapper::~socket_wrapper(){

	if (socket != INVALID_SOCKET){
			shutdown(socket, SD_RECEIVE);
			closesocket(socket);
		}
		socket = INVALID_SOCKET;
}
RemoteDesktop::SocketHandler::SocketHandler(){
	Buffer.reserve(STARTBUFFERSIZE);
}

void RemoteDesktop::SocketHandler::clear(){
	socket = std::make_shared<socket_wrapper>(INVALID_SOCKET);//reset the socket 
	memset(&addr, 0, sizeof(addr));
	bytecounter = msglength = 0;
	msgtype = NetworkMessages::INVALID;
}

