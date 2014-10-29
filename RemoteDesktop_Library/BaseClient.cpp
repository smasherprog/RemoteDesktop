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
	if (!_Connect(host, port)){
		DEBUG_MSG("socket failed with error = %d\n", WSAGetLastError());
	}
	ShutDownNetwork();
	Running = false;
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
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) return false;

	//set socket to non blocking
	u_long iMode = 1;
	ioctlsocket(ConnectSocket, FIONBIO, &iMode);
	_Socket.socket = std::make_shared<socket_wrapper>(ConnectSocket);
	OnConnect(_Socket);

	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseClient::_Run, this);

	return true;
}
void RemoteDesktop::BaseClient::_Run(){
	auto newevent = WSACreateEvent();

	WSAEventSelect(_Socket.socket.get()->socket, newevent, FD_CLOSE | FD_READ);

	WSANETWORKEVENTS NetworkEvents;

	while (Running) {
		auto Index = WSAWaitForMultipleEvents(1, &newevent, FALSE, 1000, FALSE);
		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(_Socket.socket.get()->socket, newevent, &NetworkEvents);
			if (NetworkEvents.lNetworkEvents == FD_READ
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				_OnReceive(_Socket);
			}
			else if (NetworkEvents.lNetworkEvents == FD_CLOSE){
				Running = false;
				break;
			}
		}
	}
}
void RemoteDesktop::BaseClient::_OnReceive(SocketHandler& sh){
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
void RemoteDesktop::BaseClient::Send(SOCKET s, NetworkMessages m, NetworkMsg& msg){
	Send(s, m, msg);
}

void RemoteDesktop::BaseClient::Stop() {
	Running = false;
	if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
}