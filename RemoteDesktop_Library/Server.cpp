#include "stdafx.h"
#include "Server.h"

RemoteDesktop::Server::Server(){

}
RemoteDesktop::Server::~Server(){

}

void RemoteDesktop::Server::OnConnect(){
	BaseServer::OnConnect();
}
void RemoteDesktop::Server::OnReceive(SocketHandler sh) {

}
