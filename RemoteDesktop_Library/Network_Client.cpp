#include "stdafx.h"
#include "Network_Client.h"
#include "NetworkSetup.h"
#include "SocketHandler.h"
#include "Gateway.h"
#include "NetworkProcessor.h"
#include <chrono>

RemoteDesktop::Network_Client::Network_Client(){

}
RemoteDesktop::Network_Client::~Network_Client(){
	Stop(true);
}
void RemoteDesktop::Network_Client::Setup(std::wstring port, std::wstring host){
	Stop(true);//ensure threads have been stopped
	_Dst_Host = host;
	_Dst_Port = port;
	_ShouldDisconnect = false;
	_Running = true;
}
void RemoteDesktop::Network_Client::Start(std::wstring port, std::wstring host){
	Setup(port, host);
	_BackgroundWorker = std::thread(&RemoteDesktop::Network_Client::_Run_Standard, this, -1, L"");
}
void RemoteDesktop::Network_Client::Start(std::wstring port, std::wstring host, int id, std::wstring aeskey){
	Setup(port, host);
	_BackgroundWorker = std::thread(&RemoteDesktop::Network_Client::_Run_Standard, this, id, aeskey);
}
void RemoteDesktop::Network_Client::Start(std::wstring port, std::wstring host, std::wstring gatewayurl){
	Stop(true);//ensure threads have been stopped
	Setup(port, host);
	_BackgroundWorker = std::thread(&RemoteDesktop::Network_Client::_Run_Gateway, this, gatewayurl);
}

void RemoteDesktop::Network_Client::_Run_Gateway(std::wstring gatewayurl){
	int counter = 0;

	while (_Running && ++counter < MaxConnectAttempts){
		int src_id = -1;
		std::wstring aeskey;
		DEBUG_MSG("Connecting to gateway to get id .. .");
		if (!GetGatewayID_and_Key(src_id, aeskey, gatewayurl)){
			DEBUG_MSG("Failed to connect to gateway . . ");
			continue;
		}
		DEBUG_MSG("Connected to gateway id server . . . %", src_id);


		if (OnConnectingAttempt) OnConnectingAttempt(counter, MaxConnectAttempts);

		auto sock = RemoteDesktop::Connect(_Dst_Port, _Dst_Host);
		if (sock == INVALID_SOCKET) continue;
		if (OnGatewayConnected) OnGatewayConnected(src_id);
		counter = 0;//reset timer
		MaxConnectAttempts = DEFAULTMAXCONNECTATTEMPTS;//set this to a specific value

		std::shared_ptr<SocketHandler> socket(std::make_shared<SocketHandler>(sock, true));
		socket->Exchange_Keys(-1, src_id, aeskey);
		_Run(socket);
		_HandleDisconnect(socket);
		_ShouldDisconnect = false;
	}
	if (MaxConnectAttempts >= DEFAULTMAXCONNECTATTEMPTS){
		//MessageBoxA(NULL, "Could not contact to gateway server. This could be due to your firewall blocking, or the server could be down.\n\nThe Program will exit now.", "Remote Access Tool Gateway Server Down", MB_OK);
	}
	_Running = false;
	_ShouldDisconnect = true;
}


void RemoteDesktop::Network_Client::_HandleConnect(std::shared_ptr<SocketHandler>& s){
	if (_Running && OnConnected) OnConnected(s);
	
}
void RemoteDesktop::Network_Client::_HandleViewerDisconnect(std::weak_ptr<SocketHandler>& s){
	_ShouldDisconnect = true;
}
void RemoteDesktop::Network_Client::_HandleDisconnect(std::shared_ptr<SocketHandler>& s){
	if (_Running && OnDisconnect) OnDisconnect(s);
	_ShouldDisconnect = true;
}
void RemoteDesktop::Network_Client::_HandleReceive(Packet_Header* p, const char* d, std::shared_ptr<SocketHandler>& s){
	if (_Running && OnReceived) OnReceived(p, d, s);
}

void RemoteDesktop::Network_Client::_Run_Standard(int dst_id, std::wstring aeskey){
	int counter = 0;

	while (_Running && ++counter < MaxConnectAttempts){
		if (OnConnectingAttempt) OnConnectingAttempt(counter, MaxConnectAttempts);
		DEBUG_MSG("Connecting to server . . . %", dst_id);
		auto sock = RemoteDesktop::Connect(_Dst_Port, _Dst_Host);
		if (sock == INVALID_SOCKET) continue;
		counter = 0;//reset timer

		MaxConnectAttempts = DEFAULTMAXCONNECTATTEMPTS;//set this to a specific value
		std::shared_ptr<SocketHandler> socket(std::make_shared<SocketHandler>(sock, true));
		socket->Exchange_Keys(dst_id, -1, aeskey);
		_Run(socket);
		_ShouldDisconnect = false;
	}
	std::shared_ptr<SocketHandler> emptysocket;
	if (counter >= MaxConnectAttempts && OnDisconnect)	OnDisconnect(emptysocket);//real disconnect here
	_Running = false;
	_ShouldDisconnect = true;
}

void RemoteDesktop::Network_Client::_Run(std::shared_ptr<SocketHandler>& socket){
	_Socket = socket;
	auto newevent(RAIIWSAEVENT(WSACreateEvent()));

	WSAEventSelect(socket->get_Socket(), newevent.get(), FD_CLOSE | FD_READ);
	int counter = 0;
	NetworkProcessor processor(DELEGATE(&RemoteDesktop::Network_Client::_HandleReceive), DELEGATE(&RemoteDesktop::Network_Client::_HandleConnect));
	WSANETWORKEVENTS NetworkEvents;
	DEBUG_MSG("Starting Loop");
	auto timer = std::chrono::high_resolution_clock::now();
	while (_Running && !_ShouldDisconnect) {

		auto Index = WaitForSingleObject(newevent.get(), 1000);

		if ((Index != WSA_WAIT_FAILED) && (Index != WSA_WAIT_TIMEOUT)) {

			WSAEnumNetworkEvents(socket->get_Socket(), newevent.get(), &NetworkEvents);
			if (((NetworkEvents.lNetworkEvents & FD_READ) == FD_READ)
				&& NetworkEvents.iErrorCode[FD_READ_BIT] == ERROR_SUCCESS){
				processor.Receive(socket);
			}
			else if (((NetworkEvents.lNetworkEvents & FD_CLOSE) == FD_CLOSE) && NetworkEvents.iErrorCode[FD_CLOSE_BIT] == ERROR_SUCCESS){
				break;// get out of loop and try reconnecting
			}
		}
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - timer).count() > 1000){
			if (RemoteDesktop::SocketHandler::CheckState(socket) == RemoteDesktop::Network_Return::FAILED) break;// get out of the loop and try reconnecting
			timer = std::chrono::high_resolution_clock::now();
		}

	}
	socket->Disconnect();
	DEBUG_MSG("Ending Loop");
}

RemoteDesktop::Network_Return RemoteDesktop::Network_Client::Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg, Auth_Types to_which_type){
	std::shared_ptr<SocketHandler> s(_Socket.lock());
	if (s) {
		if (to_which_type == Auth_Types::ALL) s->Send(m, msg);
		else if (to_which_type == Auth_Types::AUTHORIZED && s->Authorized) s->Send(m, msg);
		else if (to_which_type == Auth_Types::NOT_AUTHORIZED && !s->Authorized) s->Send(m, msg);
	}
	return RemoteDesktop::Network_Return::FAILED;//indicate failure
}
void RemoteDesktop::Network_Client::Stop(bool blocking) {
	_Running = false;
	_ShouldDisconnect = true;	
	BEGINTRY
		if (std::this_thread::get_id() != _BackgroundWorker.get_id() && _BackgroundWorker.joinable() && blocking) _BackgroundWorker.join();
	ENDTRY
}