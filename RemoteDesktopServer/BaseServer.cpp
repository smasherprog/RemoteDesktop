#include "stdafx.h"
#include "BaseServer.h"
#include "NetworkSetup.h"
#include "CommonNetwork.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"

RemoteDesktop::BaseServer::BaseServer(std::function<void(std::shared_ptr<SocketHandler>&)> c,
	std::function<void(Packet_Header*, const char*, std::shared_ptr<SocketHandler>&)> r,
	std::function<void(std::shared_ptr<SocketHandler>&)> d){
	Connected_CallBack = c;
	Receive_CallBack = r;
	Disconnect_CallBack = d;
	DEBUG_MSG("Starting Server");
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	SocketArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
}

RemoteDesktop::BaseServer::~BaseServer(){
	BaseServer::Stop();
	ShutDownNetwork();
	DEBUG_MSG("Stopping Server");
}
void RemoteDesktop::BaseServer::Stop(){
	Running = false;
	if (std::this_thread::get_id() != _BackGroundNetworkWorker.get_id()){
		if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	}
	for (auto& x : EventArray) {
		if (x != NULL) WSACloseEvent(x);
	}
	EventArray.resize(0);
	SocketArray.resize(0);
}

void RemoteDesktop::BaseServer::StartListening(unsigned short port, HDESK h){
	_NetworkCurrentDesktop = h;
	Stop();
	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseServer::_ListenWrapper, this, port);
}


void RemoteDesktop::BaseServer::SendToAll(NetworkMessages m, NetworkMsg& msg){
	std::lock_guard<std::mutex> lo(_SocketArrayLock);
	for (auto& c : SocketArray) {
		c->Send(m, msg);
	}
}

void RemoteDesktop::BaseServer::_ListenWrapper(unsigned short port){
	DEBUG_MSG("_ListenWrapper threadid %", std::this_thread::get_id());
	if (!_Listen(port)){
		DEBUG_MSG("socket failed with error = %d\n", WSAGetLastError());
	}
	ShutDownNetwork();
	Running = false;
}
#include <fstream>

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

	SocketArray.push_back(std::make_shared<SocketHandler>(listensocket, false));

	WSANETWORKEVENTS NetworkEvents;

	while (Running && !EventArray.empty()) {
		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);
		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(SocketArray[Index]->get_Socket(), EventArray[Index], &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT)
				&& NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				//create a new event handler for the new connect
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS) continue;// ignore this event too many connections

				_OnConnect(listensocket);
			}

			else if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ)
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				if (SocketArray[Index]->Receive() == Network_Return::FAILED) _OnDisconnect(Index);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				if (Index == 0) {//stop all processing, set running to false and next loop will fail and cleanup
					Running = false;
					continue;
				}
				_OnDisconnect(Index);
			}
		}


		if (_NetworkCurrentDesktop != NULL){
			auto threadsk = GetThreadDesktop(GetCurrentThreadId());
			if (threadsk != _NetworkCurrentDesktop){
				::SetThreadDesktop(_NetworkCurrentDesktop);
			}
		}
		//handle client disconnects here
		if (!_DisconectClientList.empty()){
			std::lock_guard<std::mutex> lock(_DisconectClientLock);
			for (auto& a : _DisconectClientList){
				auto in = -1;
				for (auto& s : SocketArray){
					++in;
					if (s.get() == a) break;
				}
				if (in != -1) _OnDisconnect(in);
			}
			_DisconectClientList.clear();
		}
	}

	Stop();
	DEBUG_MSG("_Listen Exiting");
	return true;
}

void RemoteDesktop::BaseServer::_OnDisconnect(int index){
	if (index<0 || index >= SocketArray.size()) return;
	DEBUG_MSG("_OnDisconnect Called");
	Disconnect_CallBack(SocketArray[index]);
	auto ev = EventArray[index];
	{
		std::lock_guard<std::mutex> lo(_SocketArrayLock);
		EventArray.erase(EventArray.begin() + index);
		SocketArray.erase(SocketArray.begin() + index);
	}

	if (ev != NULL) WSACloseEvent(ev);
	DEBUG_MSG("_OnDisconnect Finished");
}

void RemoteDesktop::BaseServer::_OnConnect(SOCKET listensocket){
	DEBUG_MSG("BaseServer OnConnect Called");
	int sockaddrlen = sizeof(sockaddr_in);
	sockaddr_in addr;
	auto connectsocket = accept(listensocket, (sockaddr*)&addr, &sockaddrlen);
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

	auto sock = std::make_shared<SocketHandler>(connectsocket, false);

	sock->Connected_CallBack = std::bind(&RemoteDesktop::BaseServer::_OnConnectHandler, this, std::placeholders::_1);
	sock->Receive_CallBack = std::bind(&RemoteDesktop::BaseServer::_OnReceiveHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	sock->Disconnect_CallBack = std::bind(&RemoteDesktop::BaseServer::_OnDisconnectHandler, this, std::placeholders::_1);

	auto sockptr = sock.get();
	SocketArray.push_back(sock);
	EventArray.push_back(newevent);
	sockptr->Exchange_Keys();
	DEBUG_MSG("BaseServer OnConnect End");
}
void RemoteDesktop::BaseServer::_OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s){
	for (auto& a : SocketArray){
		if (a.get() == s) {
			Receive_CallBack(p, d, a);
			break;
		}
	}
}

void RemoteDesktop::BaseServer::_OnConnectHandler(SocketHandler* socket){
	for (auto& a : SocketArray){
		if (a.get() == socket) {
			Connected_CallBack(a);
			break;
		}
	}
}

void RemoteDesktop::BaseServer::_OnDisconnectHandler(SocketHandler* socket){
	std::lock_guard<std::mutex> lock(_DisconectClientLock);
	_DisconectClientList.push_back(socket);
}
