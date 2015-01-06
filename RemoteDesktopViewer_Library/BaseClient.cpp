#include "stdafx.h"
#include "BaseClient.h"
#include "NetworkSetup.h"
#include "CommonNetwork.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"

RemoteDesktop::BaseClient::BaseClient(Delegate<void, std::shared_ptr<SocketHandler>&> c,
	Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> r,
	Delegate<void> d,
	void(__stdcall * onconnectingattempt)(int, int)) :
	Connected_CallBack(c), Receive_CallBack(r), Disconnect_CallBack(d), _OnConnectingAttempt(onconnectingattempt) {

}
RemoteDesktop::BaseClient::~BaseClient(){
	DEBUG_MSG("~BaseClient()");
	Stop();//ensure threads have been stopped
	ShutDownNetwork();

}
void RemoteDesktop::BaseClient::Connect(std::wstring host, std::wstring port, int id, std::wstring aeskey){
	Stop();//ensure threads have been stopped
	_Host = host;
	_Port = port;
	Running = true;
	_BackGroundNetworkWorker = std::thread(&BaseClient::_RunWrapper, this, id, aeskey);

}


void RemoteDesktop::BaseClient::_RunWrapper(int dst_id, std::wstring aeskey){
	int counter = 0;

	while (Running && ++counter < MaxConnectAttempts){
		DEBUG_MSG("Connect Loop % ", dst_id);
		_OnConnectingAttempt(counter, MaxConnectAttempts);
		if (!_Connect(dst_id, aeskey)){
			DEBUG_MSG("socket failed with error = %\n", WSAGetLastError());
		}
		else {
			counter = 0;//reset timer
			DisconnectReceived = false;
			MaxConnectAttempts = 15;//set this to a specific value
			_Run();
		}
	}
	if (counter >= MaxConnectAttempts)	Disconnect_CallBack();
	Running = false;
}
bool RemoteDesktop::BaseClient::_Connect(int dst_id, std::wstring aeskey){
	DEBUG_MSG("Connecting to server . . . %", dst_id);
	auto sock = RemoteDesktop::Connect(_Port, _Host);
	if (sock == INVALID_SOCKET) return false;
	Socket = std::make_shared<SocketHandler>(sock, true);

	Socket->Connected_CallBack = DELEGATE(&RemoteDesktop::BaseClient::_OnConnectHandler, this);
	Socket->Receive_CallBack = DELEGATE(&RemoteDesktop::BaseClient::_OnReceiveHandler, this);
	Socket->Disconnect_CallBack = DELEGATE(&RemoteDesktop::BaseClient::_OnDisconnectHandler, this);

	Socket->Exchange_Keys(dst_id, -1, aeskey);
	return true;
}
void RemoteDesktop::BaseClient::_Run(){
	auto newevent = WSACreateEvent();

	WSAEventSelect(Socket->get_Socket(), newevent, FD_CLOSE | FD_READ);
	int counter = 0;
	WSANETWORKEVENTS NetworkEvents;
	DEBUG_MSG("Starting Loop");

	while (Running && !DisconnectReceived) {

		auto Index = WaitForSingleObject(newevent, 1000);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(Socket->get_Socket(), newevent, &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ)
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				Socket->Receive();
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				break;// get out of loop and try reconnecting
			}
		}
		if (counter++ > 5 || Index == WSA_WAIT_TIMEOUT){
			DEBUG_MSG("Checking Timeouts!");
			Socket->CheckState();
			counter = 0;
		}


	}
	DEBUG_MSG("Ending Loop");
	WSACloseEvent(newevent);
	Socket = nullptr;
}

void RemoteDesktop::BaseClient::_OnDisconnectHandler(SocketHandler* socket){
	DisconnectReceived = true;
	DEBUG_MSG("Disconnect Received");
}
//the copies below of the socket are VERY IMPORTANT, this ensures the lifetime of the socket for the duration of the call!
void RemoteDesktop::BaseClient::_OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s){
	if (Running){
		auto s = Socket;
		if (s) Receive_CallBack(p, d, s);
	}
}
void RemoteDesktop::BaseClient::_OnConnectHandler(SocketHandler* socket){
	if (Running){
		auto s = Socket;
		if (s) Connected_CallBack(s);
	}
}

void RemoteDesktop::BaseClient::Send(NetworkMessages m, NetworkMsg& msg){
	if (Running){
		auto s = Socket;
		if (s) s->Send(m, msg);
	}
}

void RemoteDesktop::BaseClient::Stop() {
	Running = false;
	if (std::this_thread::get_id() != _BackGroundNetworkWorker.get_id()){
		if (_BackGroundNetworkWorker.joinable()) _BackGroundNetworkWorker.join();
	}
}