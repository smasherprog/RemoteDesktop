#include "stdafx.h"
#include "Network_GatewayServer.h"
#include "NetworkSetup.h"
#include "Gateway_Socket.h"

RemoteDesktop::GatewayServer::GatewayServer(void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)()): _OnConnect(onconnect), _OnDisconnect(ondisconnect) {

}
RemoteDesktop::GatewayServer::~GatewayServer(){
	Stop(true);
}

void RemoteDesktop::GatewayServer::Start(std::wstring port, std::wstring host){
	Stop(true);//ensure threads have been stopped
	_Host = host;
	_Port = port;
	_Running = true;
	_BackgroundWorker = std::thread(&RemoteDesktop::GatewayServer::_Run, this);
}
void RemoteDesktop::GatewayServer::Stop(bool blocking){
	_Running = false;
	BEGINTRY
		if (std::this_thread::get_id() != _BackgroundWorker.get_id() && _BackgroundWorker.joinable() && blocking)_BackgroundWorker.join();
	ENDTRY
}

void _HandleNewConnect(SOCKET sock, std::vector<WSAEVENT>& eventarray, std::vector<std::shared_ptr<RemoteDesktop::Gateway_Socket>>& socketarray){
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

	socketarray.push_back(std::make_shared<RemoteDesktop::Gateway_Socket>(connectsocket));
	eventarray.push_back(newevent);
	DEBUG_MSG("BaseServer OnConnect End");
}

void RemoteDesktop::GatewayServer::_Run(){

	std::vector<WSAEVENT> EventArray;
	auto socketarray = std::vector<std::shared_ptr<Gateway_Socket>>();
	EventArray.reserve(WSA_MAXIMUM_WAIT_EVENTS);
	socketarray.reserve(WSA_MAXIMUM_WAIT_EVENTS);

	SOCKET listensocket = RemoteDesktop::Listen(_Port, _Host);
	if (listensocket == INVALID_SOCKET) return;

	auto newevent = WSACreateEvent();
	WSAEventSelect(listensocket, newevent, FD_ACCEPT | FD_CLOSE);

	EventArray.push_back(newevent);
	socketarray.push_back(std::make_shared<Gateway_Socket>(listensocket));

	WSANETWORKEVENTS NetworkEvents;
	auto timer = std::chrono::high_resolution_clock::now();
	while (_Running && !EventArray.empty()) {

		auto Index = WSAWaitForMultipleEvents(EventArray.size(), EventArray.data(), FALSE, 1000, FALSE);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT) && _Running) {

			WSAEnumNetworkEvents(socketarray[Index]->ThisSocket->socket, EventArray[Index], &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT) && NetworkEvents.iErrorCode[FD_ACCEPT_BIT] == ERROR_SUCCESS){
				if (EventArray.size() >= WSA_MAXIMUM_WAIT_EVENTS - 1) continue;// ignore this event too many connections
				_HandleNewConnect(listensocket, EventArray, socketarray);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ) && NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				socketarray[Index]->Receive();
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
			_CheckForDisconnects(EventArray, socketarray);
			timer = std::chrono::high_resolution_clock::now();
		}
	}
	for (size_t beg = 1; beg < socketarray.size(); beg++){
		//OnDisconnect(sharedrockarray->at(beg));//let all callers know about the disconnect, skip slot 0 which is the listen socket
	}
	//cleanup code here
	for (auto x : EventArray) WSACloseEvent(x);
	DEBUG_MSG("_Listen Exiting");
}
void RemoteDesktop::GatewayServer::_HandleConnect(std::shared_ptr<Gateway_Socket>& ptr){

}
void RemoteDesktop::GatewayServer::_HandleDisconnect(std::shared_ptr<Gateway_Socket>& ptr){
	
}

void RemoteDesktop::GatewayServer::_CheckForDisconnects(std::vector<WSAEVENT>& eventarray, std::vector<std::shared_ptr<Gateway_Socket>>& socketarray){

	if (socketarray.size() <= 1) return;
	auto beg = socketarray.begin() + 1;
	if (beg >= socketarray.end()) return;
	while (beg != socketarray.end()){
		if (RemoteDesktop::CheckState((*beg)->ThisSocket->socket) == RemoteDesktop::Network_Return::FAILED){
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