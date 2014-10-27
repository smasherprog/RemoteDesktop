#include "stdafx.h"
#include "BaseServer.h"
#include "NetworkSetup.h"

#define STARTBUFFERSIZE 1024 *1024 *4

RemoteDesktop::BaseServer::BaseServer(){
	DEBUG_MSG("Starting Server");
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	SocketArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
}

RemoteDesktop::BaseServer::~BaseServer(){
	Stop();
	Reset();
	ShutDownNetwork();
	DEBUG_MSG("Stopping Server");
}
void RemoteDesktop::BaseServer::Stop(){
	Running = false;
	if (_BackGroundWorker.joinable()) _BackGroundWorker.join();
}

void RemoteDesktop::BaseServer::Listen(unsigned short port){
	_BackGroundWorker = std::thread(&BaseServer::_Listen, this, port);
}

void RemoteDesktop::BaseServer::_Listen(unsigned short port){
	if (!StartupNetwork()) return;
	auto listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listensocket == INVALID_SOCKET) {
		DEBUG_MSG("socket failed with error = %d\n", WSAGetLastError());
		ShutDownNetwork();
		return;

	}
	//set to non blocking
	u_long iMode = 1;
	ioctlsocket(listensocket, FIONBIO, &iMode);

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(port);
	service.sin_addr.s_addr = INADDR_ANY;


	//otherwise, try to get the 
	/*	struct addrinfo hints, *res;
		if (getaddrinfo(host, NULL, &hints, &res) != 0) {
		DEBUG_MSG("gethostbyname failed with error = %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		ShutDownNetwork();
		return false;
		}

		service.sin_addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
		*/
	if (bind(listensocket, (SOCKADDR *)& service, sizeof(SOCKADDR)) != 0) {
		DEBUG_MSG("bind failed with error = %d\n", WSAGetLastError());
		closesocket(listensocket);
		ShutDownNetwork();
		return;
	}
	auto newevent = WSACreateEvent();

	WSAEventSelect(listensocket, newevent, FD_ACCEPT | FD_CLOSE);

	if (listen(listensocket, 1) != 0) {
		DEBUG_MSG("listen failed with error = %d\n", WSAGetLastError());
		closesocket(listensocket);
		ShutDownNetwork();
		return;
	}
	EventArray.push_back(newevent);
	SocketHandler s;
	s.socket = listensocket;
	SocketArray.push_back(s);
	WSANETWORKEVENTS NetworkEvents;

	Running = true;
	while (Running && !EventArray.empty()) {
		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);
		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(SocketArray[Index].socket, EventArray[Index], &NetworkEvents);
			if (NetworkEvents.lNetworkEvents == FD_ACCEPT
				&& NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				//create a new event handler for the new connect
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS) continue;// ignore this event too many connections

				if (_OnConnect(listensocket) <= 0) continue;

				OnConnect();
			}

			else if (NetworkEvents.lNetworkEvents == FD_READ
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				_OnReceiveBegin(SocketArray[Index]);
			}
			else if (NetworkEvents.lNetworkEvents == FD_CLOSE){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					Running = false;
					continue;
				}

			}
		}
	}
	Reset();
}
void RemoteDesktop::BaseServer::OnConnect(){
	if (_OnConnectCB != nullptr) _OnConnectCB();
}
void RemoteDesktop::BaseServer::Reset(){
	Running = false;
	for (auto& a : EventArray){
		if (a != NULL) WSACloseEvent(a);
	}
	for (auto& a : SocketArray){
		if (a.socket != INVALID_SOCKET) closesocket(a.socket);
	}
	EventArray.resize(0);
	SocketArray.resize(0);
}
int RemoteDesktop::BaseServer::_ProcessPacketHeader(SocketHandler& sh){
	if (sh.msgtype == INVALID){//new message, read the header
		if (sh.bytecounter < NETWORKHEADERSIZE){//only read more if any is needed
			auto amtrec = recv(sh.socket, sh.Buffer.data() + sh.bytecounter, NETWORKHEADERSIZE - sh.bytecounter, 0);
			if (amtrec > 0){//received data.. yay!
				sh.bytecounter += amtrec;
			}
			else return -1;//let the caller know to stop processing
		}//check if there is enough data in to complete the header 
		if (sh.bytecounter >= NETWORKHEADERSIZE){//msg length and type received
			memcpy(&sh.msglength, sh.Buffer.data(), sizeof(int));//copy length over
			unsigned char msgtype = 0;
			memcpy(&msgtype, sh.Buffer.data() + sizeof(int), sizeof(msgtype));//copy msg type over
			sh.msgtype = (NetworkMessages)msgtype;
			sh.bytecounter -= NETWORKHEADERSIZE;
			if (sh.bytecounter > 0){//extra data was received some how........ make sure to move it back in the array
				memmove(sh.Buffer.data(), sh.Buffer.data() + NETWORKHEADERSIZE, sh.bytecounter);//use memove in case of overlapping copy
			}
			if (sh.msgtype == PING) {//reset the packet.. this is just a dummy to trigger disconnect events
				sh.msglength = 0;
				sh.msgtype = INVALID;
				//there is no payload with a PING packet, so it just needs to be reset
				return _ProcessPacketHeader(sh);//keep proccessing in case there is more data to go
			}
			return 1;// keep processing the packet
		}
		else return -1;// header not done and no data to build it.. stop processing
	}

}


int RemoteDesktop::BaseServer::_OnReceiveBegin(SocketHandler& sh){
	while (true){
		auto result = _ProcessPacketHeader(sh);// assemble header info
		if (result == 1){//if there is header info...
			result = _ProcessPacketBody(sh);//process the body of the message
			if (result == 1) {//if the message is complete, then call on receive
				OnReceive(sh);//
				_OnReceiveEnd(sh);//this will remove the data from the previous message
			}
			else break;
		}
		else break;//get out done  no more data to process here
	}
	return 1;
}
int RemoteDesktop::BaseServer::_ProcessPacketBody(SocketHandler& sh){

	auto amtrec = recv(sh.socket, sh.Buffer.data() + sh.bytecounter, sh.msglength - sh.bytecounter, 0);
	if (amtrec > 0){
		sh.bytecounter += amtrec;
		if (sh.bytecounter >= sh.msglength) return 1;// message complete
	}
	return -1;// not done..
}
void RemoteDesktop::BaseServer::_OnReceiveEnd(SocketHandler& sh){
	if (sh.bytecounter > sh.msglength){// more data in the buffer than was in the message
		memmove(sh.Buffer.data(), sh.Buffer.data() + sh.msglength, sh.bytecounter - sh.msglength);
		sh.bytecounter -= sh.msglength;
	}
	else sh.bytecounter = 0;
	sh.msglength = 0;
	sh.msgtype = INVALID;
}
int RemoteDesktop::BaseServer::_OnConnect(SOCKET listensocket){
	int sockaddrlen = sizeof(sockaddr_in);
	SocketHandler so;
	so.socket = accept(listensocket, (struct sockaddr*)&so.addr, &sockaddrlen);
	if (so.socket == INVALID_SOCKET) return -1;
	auto newevent = WSACreateEvent();
	if (newevent == WSA_INVALID_EVENT) return -1;
	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(so.socket, FIONBIO, &iMode);

	so.Buffer.reserve(STARTBUFFERSIZE);//reserve space for data
	SocketArray.push_back(so);
	WSAEventSelect(so.socket, newevent, FD_READ | FD_CLOSE);
	EventArray.push_back(newevent);
	return 1;
}
