#include "stdafx.h"
#include "Network_Server.h"
#include "NetworkSetup.h"
#include "SocketHandler.h"
#include <string>
#include "NetworkProcessor.h"
#include <chrono>

RemoteDesktop::Network_Server::Network_Server(){
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
		if (std::this_thread::get_id() != _BackgroundWorker.get_id() && _BackgroundWorker.joinable() && blocking)_BackgroundWorker.join();
	ENDTRY
}
void RemoteDesktop::Network_Server::_Run(){

	std::vector<WSAEVENT> EventArray;
	auto sharedrockarray = std::make_shared<std::vector<std::shared_ptr<SocketHandler>>>();
	_Sockets = sharedrockarray;
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	sharedrockarray->reserve(WSA_MAXIMUM_WAIT_EVENTS);

	SOCKET listensocket = RemoteDesktop::Listen(_Port, _Host);
	if (listensocket == INVALID_SOCKET) return;

	auto newevent = WSACreateEvent();
	WSAEventSelect(listensocket, newevent, FD_ACCEPT | FD_CLOSE);

	EventArray.push_back(newevent);
	sharedrockarray->push_back(std::make_shared<SocketHandler>(listensocket, false));

	NetworkProcessor processor(DELEGATE(&RemoteDesktop::Network_Server::_HandleReceive), DELEGATE(&RemoteDesktop::Network_Server::_HandleConnect));
	WSANETWORKEVENTS NetworkEvents;
	auto timer = std::chrono::high_resolution_clock::now();
	while (_Running && !EventArray.empty()) {

		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT) && _Running) {

			WSAEnumNetworkEvents(sharedrockarray->at(Index)->get_Socket(), EventArray[Index], &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT) && NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS - 1) continue;// ignore this event too many connections
				_HandleNewConnect(listensocket, EventArray, *sharedrockarray);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ) && NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				processor.Receive(sharedrockarray->at(Index));
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					_Running = false;
					continue;
				}
			}
		}
		//once every second send a keep alive. this will trigger disconnects 
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - timer).count() > 1000){
			_CheckForDisconnects(EventArray, *sharedrockarray);
			timer = std::chrono::high_resolution_clock::now();
		}
	}
	for (size_t beg = 1; beg < sharedrockarray->size(); beg++){
		OnDisconnect(sharedrockarray->at(beg));//let all callers know about the disconnect, skip slot 0 which is the listen socket
	}
	//cleanup code here
	for (auto x : EventArray) WSACloseEvent(x);
	DEBUG_MSG("_Listen Exiting");
}
RemoteDesktop::Network_Return RemoteDesktop::Network_Server::Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg, Auth_Types to_which_type){
	auto sockarr(_Sockets.lock());
	if (sockarr){
	
		for (size_t beg = 1; beg < sockarr->size() && beg < sockarr->capacity() - 1; beg++){
			auto s(sockarr->at(beg));//get a copy
			if (s) {
				if (to_which_type == Auth_Types::ALL) s->Send(m, msg);
				else if (to_which_type == Auth_Types::AUTHORIZED && s->Authorized) s->Send(m, msg);
				else if (to_which_type == Auth_Types::NOT_AUTHORIZED && !s->Authorized) s->Send(m, msg);
			}
		}
	}

	return RemoteDesktop::Network_Return::COMPLETED;
}
void RemoteDesktop::Network_Server::_HandleConnect(std::shared_ptr<SocketHandler>& s){
	if (_Running && OnConnected)  OnConnected(s);
}
void RemoteDesktop::Network_Server::_HandleDisconnect(std::shared_ptr<SocketHandler>& s){
	if (_Running && OnDisconnect) OnDisconnect(s);
}
void RemoteDesktop::Network_Server::_HandleReceive(Packet_Header* p, const char* d, std::shared_ptr<SocketHandler>& s){
	if (_Running && OnReceived) OnReceived(p, d, s);
}

void RemoteDesktop::Network_Server::_HandleNewConnect(SOCKET sock, std::vector<WSAEVENT>& eventarray, std::vector<std::shared_ptr<SocketHandler>>& socketarray){
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

	socketarray.push_back(newsocket);
	eventarray.push_back(newevent);
	newsocket->Exchange_Keys(-1, -1, L"");
	DEBUG_MSG("BaseServer OnConnect End");
}
void RemoteDesktop::Network_Server::_CheckForDisconnects(std::vector<WSAEVENT>& eventarray, std::vector<std::shared_ptr<SocketHandler>>& socketarray){

	if (socketarray.size() <= 1) return;
	auto beg = socketarray.begin() + 1;
	if (beg >= socketarray.end()) return;
	while (beg != socketarray.end()){
		if (RemoteDesktop::SocketHandler::CheckState(*beg) == RemoteDesktop::Network_Return::FAILED){
			_HandleDisconnect(*beg);
			//get the index before deletion
			int index = beg - socketarray.begin();
			beg = socketarray.erase(beg);
			WSACloseEvent(eventarray[index]);
			eventarray.erase(eventarray.begin() + index);
			continue;
		}
		++beg;
	}

}