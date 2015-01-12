#include "stdafx.h"
#include "Network_Server.h"
#include "NetworkSetup.h"
#include "SocketHandler.h"
#include <string>

RemoteDesktop::Network_Server::Network_Server(){
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	SocketArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	DEBUG_MSG("Starting Server");
}
RemoteDesktop::Network_Server::~Network_Server(){
	Stop(true);
}

void RemoteDesktop::Network_Server::Start(std::wstring port, std::wstring host){
	Stop(true);//ensure threads have been stopped
	_Host = host;
	_Port = port;
	_Running = true;
	_BackgroundWorker = std::thread(&RemoteDesktop::Network_Server::_Run, this);
}

void RemoteDesktop::Network_Server::Stop(bool blocking) {
	_Running = false;
	BEGINTRY
		if (std::this_thread::get_id() != _BackgroundWorker.get_id() &&  _BackgroundWorker.joinable() && blocking)_BackgroundWorker.join();

	ENDTRY
		for (auto x : EventArray) {
			if (x != NULL) WSACloseEvent(x);
		}
	EventArray.resize(0);
	SocketArray.resize(0);
}
void RemoteDesktop::Network_Server::_Run(){

	SOCKET listensocket = RemoteDesktop::Listen(_Port, _Host);
	if (listensocket == INVALID_SOCKET) return;

	auto newevent = WSACreateEvent();
	WSAEventSelect(listensocket, newevent, FD_ACCEPT | FD_CLOSE);

	EventArray.push_back(newevent);
	SocketArray.push_back(std::make_shared<SocketHandler>(listensocket, false));

	WSANETWORKEVENTS NetworkEvents;
	int counter = 0;
	//NetworkProcessor processor;
	while (_Running && !EventArray.empty()) {

		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT) && Index < SocketArray.size()) {

			WSAEnumNetworkEvents(SocketArray[Index]->get_Socket(), EventArray[Index], &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT) && NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS - 1) continue;// ignore this event too many connections
				_HandleNewConnect(listensocket);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ) && NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				SocketArray[Index]->Receive();
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					_Running = false;
					continue;
				}
			}
		}
		if (counter++ > 5 || Index == WSA_WAIT_TIMEOUT){
			_CheckForDisconnects();
			counter = 0;
		}
	}

	for (size_t beg = 1; beg < SocketArray.size(); beg++){
		OnDisconnect(SocketArray[beg]);//let all callers know about the disconnect, skip slot 0 which is the listen socket
	}
	//cleanup code here
	for (auto x : EventArray) WSACloseEvent(x);
	EventArray.resize(0);
	SocketArray.resize(0);
	DEBUG_MSG("_Listen Exiting");
}
RemoteDesktop::Network_Return RemoteDesktop::Network_Server::Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg){
	for (size_t beg = 1; beg < SocketArray.size() && beg < SocketArray.capacity() - 1; beg++){
		auto s(SocketArray[beg]);//get a copy
		if (s) s->Send(m, msg);
	}
	return RemoteDesktop::Network_Return::COMPLETED;
}
void RemoteDesktop::Network_Server::_HandleConnect(SocketHandler* ptr){
	if (_Running) {
		for (size_t beg = 1; beg < SocketArray.size() && beg < SocketArray.capacity() - 1; beg++){
			if (ptr == SocketArray[beg].get()){
				auto s(SocketArray[beg]);//get a copy
				if (s && OnConnected)  OnConnected(s);
			}
		}
	}
}
void RemoteDesktop::Network_Server::_HandleDisconnect(SocketHandler* ptr){
	if (_Running) {
		for (size_t beg = 1; beg < SocketArray.size() && beg < SocketArray.capacity() - 1; beg++){
			if (ptr == SocketArray[beg].get()){
				auto s(SocketArray[beg]);//get a copy
				if (s && OnDisconnect) OnDisconnect(s);
			}
		}
	}
}
void RemoteDesktop::Network_Server::_HandleReceive(Packet_Header* p, const char* d, SocketHandler* ptr){
	if (_Running) {
		for (size_t beg = 1; beg < SocketArray.size() && beg < SocketArray.capacity() - 1; beg++){
			if (ptr == SocketArray[beg].get()){
				auto s(SocketArray[beg]);//get a copy
				if (s && OnReceived) OnReceived(p, d, s);
			}
		}
	}
}

void RemoteDesktop::Network_Server::_HandleNewConnect(SOCKET sock){
	DEBUG_MSG("BaseServer OnConnect Called");
	int sockaddrlen = sizeof(sockaddr_in);
	sockaddr_in addr;
	auto connectsocket = accept(sock, (sockaddr*)&addr, &sockaddrlen);
	if (connectsocket == INVALID_SOCKET) return;
	auto newevent = WSACreateEvent();
	if (newevent == WSA_INVALID_EVENT){
		closesocket(connectsocket);
		return;
	}
	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(connectsocket, FIONBIO, &iMode);
	WSAEventSelect(connectsocket, newevent, FD_READ | FD_CLOSE);

	auto newsocket = std::make_shared<SocketHandler>(connectsocket, false);

	newsocket->Connected_CallBack = std::bind(&RemoteDesktop::Network_Server::_HandleConnect, this, std::placeholders::_1);
	newsocket->Receive_CallBack = std::bind(&RemoteDesktop::Network_Server::_HandleReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	newsocket->Disconnect_CallBack = std::bind(&RemoteDesktop::Network_Server::_HandleDisconnect, this, std::placeholders::_1);

	SocketArray.push_back(newsocket);
	EventArray.push_back(newevent);
	newsocket->Exchange_Keys(-1, -1, L"");
	DEBUG_MSG("BaseServer OnConnect End");
}
void RemoteDesktop::Network_Server::_CheckForDisconnects(){
	if (SocketArray.size()<=1) return;
	auto beg = SocketArray.begin() + 1;
	if (beg >= SocketArray.end()) return;
	while (beg != SocketArray.end()){
		//checkstate will block if a disconnect occurs until processing stops, which means no receives can occur if the socket is in a disconnect state.
		//it will also do the disconnect call back to notify anyone before the socket is destoryed in the following lines of code
		if ((*beg)->CheckState() == RemoteDesktop::Network_Return::FAILED){
			//get the index before deletion
			int index = beg - SocketArray.begin();
			beg = SocketArray.erase(beg);
			WSACloseEvent(EventArray[index]);
			EventArray.erase(EventArray.begin() + index);
			continue;
		}
		++beg;
	}

}