#include "stdafx.h"
#include "NetworkSetup.h"

bool RemoteDesktop::_INTERNAL::NetworkStarted = false;

bool RemoteDesktop::StartupNetwork(){
	if (RemoteDesktop::_INTERNAL::NetworkStarted) return RemoteDesktop::_INTERNAL::NetworkStarted;
	WSADATA wsaData = { 0 };
	RemoteDesktop::_INTERNAL::NetworkStarted = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
	return RemoteDesktop::_INTERNAL::NetworkStarted;
}
void RemoteDesktop::ShutDownNetwork(){
	WSACleanup();
	RemoteDesktop::_INTERNAL::NetworkStarted = false;
}
void RemoteDesktop::StandardSocketSetup(SOCKET socket){
	//set to non blocking
	u_long iMode = 1;
	ioctlsocket(socket, FIONBIO, &iMode);
	//set no delay 
	BOOL nodly = TRUE;
	if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodly, sizeof(nodly)) == SOCKET_ERROR){
		auto errmsg = WSAGetLastError();
		DEBUG_MSG("failed to sent TCP_NODELY with error = %", errmsg);
	}
}
SOCKET RemoteDesktop::Connect(std::wstring port, std::wstring host){
	if (!StartupNetwork()) return INVALID_SOCKET;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfoW *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port

	auto iResult = GetAddrInfoW(host.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0) return INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) return INVALID_SOCKET;
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

	FreeAddrInfoW(result);

	if (ConnectSocket == INVALID_SOCKET) return INVALID_SOCKET;
	StandardSocketSetup(ConnectSocket);

	auto newevent = WSACreateEvent();
	WSAEventSelect(ConnectSocket, newevent, FD_CONNECT);
	auto Index = WSAWaitForMultipleEvents(1, &newevent, TRUE, 1000, FALSE);
	WSANETWORKEVENTS NetworkEvents;
	WSAEnumNetworkEvents(ConnectSocket, newevent, &NetworkEvents);
	WSACloseEvent(newevent);

	if ((Index == WSA_WAIT_FAILED) || (Index == WSA_WAIT_TIMEOUT)) {
		DEBUG_MSG("Connect Failed!");
		return INVALID_SOCKET;
	}
	return ConnectSocket;
}