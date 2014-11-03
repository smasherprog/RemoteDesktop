#include "stdafx.h"
#include "BaseClient.h"
#include "NetworkSetup.h"
#include "CommonNetwork.h"

RemoteDesktop::BaseClient::BaseClient(){

}
RemoteDesktop::BaseClient::~BaseClient(){
	Running = false;
	ShutDownNetwork();
	DEBUG_MSG("Stopping Client");
}
void RemoteDesktop::BaseClient::Connect(const char* host, const char* port){
	Stop();//ensure threads have been stopped
	if (!_Connect(host, port)){
		DEBUG_MSG("socket failed with error = %\n", WSAGetLastError());
		ShutDownNetwork();
		Running = false;
	}

}

bool RemoteDesktop::BaseClient::_Connect(const char* host, const char* port){
	if (!StartupNetwork()) return false;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	auto iResult = getaddrinfo(host, port, &hints, &result);
	if (iResult != 0) return false;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) return false;
		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			DEBUG_MSG("Server Down....");
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) return false;

	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(ConnectSocket, FIONBIO, &iMode);
	BOOL nodly = TRUE;
	if (setsockopt(ConnectSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodly, sizeof(nodly)) == SOCKET_ERROR){
		auto errmsg = WSAGetLastError();
		DEBUG_MSG("failed to sent TCP_NODELY with error = %", errmsg);
	}



	auto newevent = WSACreateEvent();
	WSAEventSelect(ConnectSocket, newevent, FD_CONNECT);
	auto Index = WSAWaitForMultipleEvents(1, &newevent, TRUE, 1000, FALSE);
	WSANETWORKEVENTS NetworkEvents;
	WSAEnumNetworkEvents(ConnectSocket, newevent, &NetworkEvents);

	if ((Index == WSA_WAIT_FAILED) || (Index == WSA_WAIT_TIMEOUT)) {
		DEBUG_MSG("Connect Failed!");
		return false;
	}
	else if (((NetworkEvents.lNetworkEvents & FD_CONNECT) == FD_CONNECT)
		&& NetworkEvents.iErrorCode[FD_CONNECT_BIT] == ERROR_SUCCESS){
		OnConnect(_Socket);
	}
	else {
		DEBUG_MSG("Connect Failed!");
		return false;
	}

	WSACloseEvent(newevent);

	_Socket.socket = std::make_shared<socket_wrapper>(ConnectSocket);
	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseClient::_Run, this);
	OnConnect(_Socket);
	return true;
}

void RemoteDesktop::BaseClient::_Run(){
	auto newevent = WSACreateEvent();

	WSAEventSelect(_Socket.socket->socket, newevent, FD_CLOSE | FD_READ);

	WSANETWORKEVENTS NetworkEvents;
	DEBUG_MSG("Starting Loop");

	while (Running) {

		auto Index = WSAWaitForMultipleEvents(1, &newevent, FALSE, 1000, FALSE);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(_Socket.socket->socket, newevent, &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ)
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				_OnReceive(_Socket);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				Running = false;// loop will end and disconnect will be called
			}
		}
	}
	WSACloseEvent(newevent);
	_OnDisconnect(_Socket);

	DEBUG_MSG("Ending Loop");
}

void RemoteDesktop::BaseClient::_OnDisconnect(SocketHandler& sh){
	Running = false;
	DEBUG_MSG("Disconnect Received");
	OnDisconnect(sh);
	_Socket.clear();//shut down socket
}

void RemoteDesktop::BaseClient::_OnReceive(SocketHandler& sh){
	//DEBUG_MSG("_OnReceive Called");
	auto result = 1;
	while (true){
		result = RemoteDesktop::_INTERNAL::_ProcessPacketHeader(sh);// assemble header info
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

int RemoteDesktop::BaseClient::Send(NetworkMessages m, NetworkMsg& msg){
	return RemoteDesktop::_INTERNAL::_Send(_Socket.socket->socket, m, msg);
}

void RemoteDesktop::BaseClient::Stop() {
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	_Socket.clear();//shut down socket
}