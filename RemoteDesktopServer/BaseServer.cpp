#include "stdafx.h"
#include "BaseServer.h"
#include "NetworkSetup.h"
#include "CommonNetwork.h"



RemoteDesktop::BaseServer::BaseServer(){
	DEBUG_MSG("Starting Server");
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	SocketArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
}

RemoteDesktop::BaseServer::~BaseServer(){
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	Stop();
	ShutDownNetwork();
	DEBUG_MSG("Stopping Server");
}
void RemoteDesktop::BaseServer::Stop(){
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	for (auto& x : EventArray) {
		if (x != NULL) WSACloseEvent(x);
	}
	EventArray.resize(0);
	SocketArray.resize(0);
}

void RemoteDesktop::BaseServer::Listen(unsigned short port){
	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseServer::_ListenWrapper, this, port);
}

int RemoteDesktop::BaseServer::Send(SOCKET s, NetworkMessages m, NetworkMsg& msg){
	return RemoteDesktop::_INTERNAL::_Send(s, m, msg);
}
void RemoteDesktop::BaseServer::SendToAll(NetworkMessages m, NetworkMsg& msg){
	for (auto i = 1; i < SocketArray.size(); i++){
		auto tmp = SocketArray[i].socket;// get a copy to hold the lock and prevent closing the socket
		auto sock = tmp->socket;
		if (Send(sock, m, msg) == -1) _OnDisconnect(i);//disconect cliet here
	}
}

void RemoteDesktop::BaseServer::_ListenWrapper(unsigned short port){
	if (!_Listen(port)){
		DEBUG_MSG("socket failed with error = %d\n", WSAGetLastError());
	}
	ShutDownNetwork();
	Running = false;
}
bool RemoteDesktop::BaseServer::_Listen(unsigned short port){
	if (!StartupNetwork()) return false;
	auto listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listensocket == INVALID_SOCKET) return false;

	//set to non blocking
	u_long iMode = 1;
	ioctlsocket(listensocket, FIONBIO, &iMode);

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(port);
	service.sin_addr.s_addr = INADDR_ANY;

	if (bind(listensocket, (SOCKADDR *)& service, sizeof(SOCKADDR)) != 0) {
		closesocket(listensocket);
		return false;
	}
	auto newevent = WSACreateEvent();

	WSAEventSelect(listensocket, newevent, FD_ACCEPT | FD_CLOSE);

	if (listen(listensocket, 1) != 0) {
		closesocket(listensocket);
		return false;
	}
	EventArray.push_back(newevent);
	SocketHandler s;
	s.socket = std::make_shared<socket_wrapper>(listensocket);
	SocketArray.push_back(s);
	WSANETWORKEVENTS NetworkEvents;

	while (Running && !EventArray.empty()) {
		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);
		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(SocketArray[Index].socket->socket, EventArray[Index], &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT)
				&& NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				//create a new event handler for the new connect
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS) continue;// ignore this event too many connections

				_OnConnect(listensocket);
			}

			else if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ)
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				_OnReceive(SocketArray[Index]);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					Running = false;
					continue;
				}
				_OnDisconnect(Index);
			}
		}
	}
	Stop();
	return true;
}

void RemoteDesktop::BaseServer::_OnDisconnect(int index){
	DEBUG_MSG("_OnDisconnect Called");
	OnDisconnect(SocketArray[index]);
	SocketArray.erase(SocketArray.begin() + index);
	if (EventArray[index] != NULL) WSACloseEvent(EventArray[index]);
	EventArray.erase(EventArray.begin() + index);
	DEBUG_MSG("_OnDisconnect Finished");
}


void RemoteDesktop::BaseServer::_OnReceive(SocketHandler& sh){
	
	while (true){
		//DEBUG_MSG("_OnReceive Called");
		auto result = RemoteDesktop::_INTERNAL::_ProcessPacketHeader(sh);// assemble header info
		if (result == 1){//if there is header info...
			result = RemoteDesktop::_INTERNAL::_ProcessPacketBody(sh);//process the body of the message
			if (result == 1) {//if the message is complete, then call on receive
				OnReceive(sh);
				RemoteDesktop::_INTERNAL::_RecevieEnd(sh);
			}
			else break;
		}
		else break;//get out done  no more data to process here
	}
	//DEBUG_MSG("_OnReceive Finished");
}


void RemoteDesktop::BaseServer::_OnConnect(SOCKET listensocket){
	DEBUG_MSG("OnConnect Called");
	int sockaddrlen = sizeof(sockaddr_in);
	SocketHandler so;
	so.socket = std::make_shared<socket_wrapper>(accept(listensocket, (struct sockaddr*)&so.addr, &sockaddrlen));
	if (so.socket->socket == INVALID_SOCKET) return;
	auto newevent = WSACreateEvent();
	if (newevent == WSA_INVALID_EVENT) return;
	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(so.socket->socket, FIONBIO, &iMode);

	SocketArray.push_back(so);

	WSAEventSelect(so.socket->socket, newevent, FD_READ | FD_CLOSE);
	EventArray.push_back(newevent);
	auto ind = EventArray.size() - 1;
	SocketArray[ind].Buffer.reserve(STARTBUFFERSIZE);// the copy into the array does not preserve capacity
	OnConnect(SocketArray[ind]);
	DEBUG_MSG("OnConnect Success");
}
