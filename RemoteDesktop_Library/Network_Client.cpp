#include "stdafx.h"
#include "Network_Client.h"
#include "NetworkSetup.h"

RemoteDesktop::Network_Client::Network_Client(){

}
RemoteDesktop::Network_Client::~Network_Client(){
	Stop(true);
}
void RemoteDesktop::Network_Client::Start(std::wstring port, std::wstring host){
	Stop(true);//complete stop
	_Dst_Host = host;
	_Dst_Port = port;
	_Running = true;
	_BackgroundWorker = std::thread(&Network_Client::_Run, this);

}
void RemoteDesktop::Network_Client::Stop(bool blocking){
	_Running = false;
	if (blocking){
		if (std::this_thread::get_id() != _BackgroundWorker.get_id()){
			if (_BackgroundWorker.joinable()) _BackgroundWorker.join();
		}
	}
}
void RemoteDesktop::Network_Client::Send(NetworkMessages m, const NetworkMsg& msg){

}
void RemoteDesktop::Network_Client::_Run(){
	auto connectioncounter = 0;
	while (_Running && connectioncounter++ < _MaxConnectAttempts){
		SOCKET sock = RemoteDesktop::Connect(_Dst_Port, _Dst_Host);
		if (sock != INVALID_SOCKET){
			

		}
	}
	if (connectioncounter >= _MaxConnectAttempts) OnDisconnect();
	_Running = false;
}
