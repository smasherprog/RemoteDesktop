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
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	Reset();
	ShutDownNetwork();
	DEBUG_MSG("Stopping Server");
}
void RemoteDesktop::BaseServer::Stop(){
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
}

void RemoteDesktop::BaseServer::Listen(unsigned short port){
	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseServer::_ListenWrapper, this, port);
}
void _SendLoop(SOCKET s, char* data, int len){
	while (len > 0){
		auto sentamount = send(s, data, len, 0);
		if (sentamount == SOCKET_ERROR) 
			return DEBUG_MSG("send failed with error = %d\n", WSAGetLastError());
		len -= sentamount;
	}
}
void RemoteDesktop::BaseServer::Send(SOCKET s, NetworkMessages m, NetworkMsg& msg){
	if (s != INVALID_SOCKET){
		auto payloadlen = msg.payloadlength();
		_SendLoop(s, (char*)&payloadlen, sizeof(payloadlen));//send the lenth first
		auto header = (char)m;
		_SendLoop(s, &header, sizeof(header)); // send header
		for (auto i = 0; i < msg.data.size(); i++){
			_SendLoop(s, msg.data[i], msg.lens[i]);//send the payload
		}
	}
}
void RemoteDesktop::BaseServer::SendToAll(NetworkMessages m, NetworkMsg& msg){
	for (auto i = 0; i<SocketArray.size(); i++){
		auto tmp = SocketArray[i].socket;// get a copy to hold the lock and prevent closing the socket
		auto sock = tmp.get()->socket;
		Send(sock, m, msg);
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

			WSAEnumNetworkEvents(SocketArray[Index].socket.get()->socket, EventArray[Index], &NetworkEvents);
			if (NetworkEvents.lNetworkEvents == FD_ACCEPT
				&& NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				//create a new event handler for the new connect
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS) continue;// ignore this event too many connections

				_OnConnect(listensocket);
			}

			else if (NetworkEvents.lNetworkEvents == FD_READ
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				_OnReceive(SocketArray[Index]);
			}
			else if (NetworkEvents.lNetworkEvents == FD_CLOSE){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					Running = false;
					continue;
				}
				_OnDisconnect(Index);
			}
		}
	}
	Reset();
	return true;
}

void RemoteDesktop::BaseServer::_OnDisconnect(int index){
	auto& sh = SocketArray[index];
	OnDisconnect(sh);
	SocketArray.erase(SocketArray.begin() + index);
	if (EventArray[index] != NULL) WSACloseEvent(EventArray[index]);
	EventArray.erase(EventArray.begin() + index);

}
void RemoteDesktop::BaseServer::Reset(){
	Running = false;
	for (auto& x : EventArray) {
		if (x!= NULL) WSACloseEvent(x);
	}
	EventArray.resize(0);
	SocketArray.resize(0);
}

int _ProcessPacketHeader(RemoteDesktop::SocketHandler& sh){

	if (sh.msgtype == RemoteDesktop::NetworkMessages::INVALID){//new message, read the header
		if (sh.bytecounter < NETWORKHEADERSIZE){//only read more if any is needed
			auto amtrec = recv(sh.socket.get()->socket , sh.Buffer.data() + sh.bytecounter, NETWORKHEADERSIZE - sh.bytecounter, 0);
			if (amtrec > 0){//received data.. yay!
				sh.bytecounter += amtrec;
			}
			else return -1;//let the caller know to stop processing
		}//check if there is enough data in to complete the header 
		if (sh.bytecounter >= NETWORKHEADERSIZE){//msg length and type received
			memcpy(&sh.msglength, sh.Buffer.data(), sizeof(int));//copy length over
			unsigned char msgtype = 0;
			memcpy(&msgtype, sh.Buffer.data() + sizeof(int), sizeof(msgtype));//copy msg type over
			sh.msgtype = (RemoteDesktop::NetworkMessages)msgtype;
			sh.bytecounter -= NETWORKHEADERSIZE;
			if (sh.bytecounter > 0){//extra data was received some how........ make sure to move it back in the array
				memmove(sh.Buffer.data(), sh.Buffer.data() + NETWORKHEADERSIZE, sh.bytecounter);//use memove in case of overlapping copy
			}
			if (sh.msgtype == RemoteDesktop::NetworkMessages::PING) {//reset the packet.. this is just a dummy to trigger disconnect events
				sh.msglength = 0;
				sh.msgtype = RemoteDesktop::NetworkMessages::INVALID;
				//there is no payload with a PING packet, so it just needs to be reset
				return _ProcessPacketHeader(sh);//keep proccessing in case there is more data to go
			}
			return 1;// keep processing the packet
		}
		else return -1;// header not done and no data to build it.. stop processing
	}
	return 1;
}
int _ProcessPacketBody(RemoteDesktop::SocketHandler& sh){
	auto amtrec = recv(sh.socket.get()->socket, sh.Buffer.data() + sh.bytecounter, sh.msglength - sh.bytecounter, 0);
	if (amtrec > 0){
		sh.bytecounter += amtrec;
		if (sh.bytecounter >= sh.msglength) return 1;// message complete
	}
	return -1;// not done..
}
void RecevieEnd(RemoteDesktop::SocketHandler& sh){
	if (sh.bytecounter > sh.msglength){// more data in the buffer than was in the message
		memmove(sh.Buffer.data(), sh.Buffer.data() + sh.msglength, sh.bytecounter - sh.msglength);
		sh.bytecounter -= sh.msglength;
	}
	else sh.bytecounter = 0;
	sh.msglength = 0;
	sh.msgtype = RemoteDesktop::NetworkMessages::INVALID;
}
void RemoteDesktop::BaseServer::_OnReceive(SocketHandler& sh){
	while (true){
		auto result = _ProcessPacketHeader(sh);// assemble header info
		if (result == 1){//if there is header info...
			result = _ProcessPacketBody(sh);//process the body of the message
			if (result == 1) {//if the message is complete, then call on receive
				OnReceive(sh);
				RecevieEnd(sh);
			}
			else break;
		}
		else break;//get out done  no more data to process here
	}
}


void RemoteDesktop::BaseServer::_OnConnect(SOCKET listensocket){
	int sockaddrlen = sizeof(sockaddr_in);
	SocketHandler so;
	so.socket = std::make_shared<socket_wrapper>(accept(listensocket, (struct sockaddr*)&so.addr, &sockaddrlen));
	if (so.socket.get()->socket == INVALID_SOCKET) return;
	auto newevent = WSACreateEvent();
	if (newevent == WSA_INVALID_EVENT) return;
	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(so.socket.get()->socket, FIONBIO, &iMode);

	so.Buffer.reserve(STARTBUFFERSIZE);//reserve space for data
	SocketArray.push_back(so);
	WSAEventSelect(so.socket.get()->socket, newevent, FD_READ | FD_CLOSE);
	EventArray.push_back(newevent);
	auto ind = EventArray.size() - 1;
	OnConnect(SocketArray[ind]);
	if (_OnConnectCB != nullptr) _OnConnectCB();
}
