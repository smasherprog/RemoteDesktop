#include "stdafx.h"
#include "NetworkProcessor.h"
#include "SocketHandler.h"
#include "Desktop_Monitor.h"
#include "NetworkSetup.h"
#include <future>

RemoteDesktop::NetworkProcessor::NetworkProcessor(Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&>& receive_callback, Delegate<void, std::shared_ptr<SocketHandler>&>& onconnect_callback):
_Receive_callback(receive_callback), _Onconnect_callback(onconnect_callback) {
	_Running = true;
	_Worker = std::thread(&RemoteDesktop::NetworkProcessor::_Run, this);
}
RemoteDesktop::NetworkProcessor::~NetworkProcessor(){
	_Running = false;
	_Queue.ShutDown();
	BEGINTRY
		if (std::this_thread::get_id() != _Worker.get_id() && _Worker.joinable()) _Worker.join();
	ENDTRY
}

void RemoteDesktop::NetworkProcessor::_Run(){
	DesktopMonitor _DesktopMonitor;
	while (_Running){
		if (!_DesktopMonitor.Is_InputDesktopSelected()) _DesktopMonitor.Switch_to_Desktop(DesktopMonitor::Desktops::INPUT);
		auto socket(_Queue.pop());
		if (socket) RemoteDesktop::SocketHandler::ProcessReceived(socket, _Receive_callback ,_Onconnect_callback);
	}
	DEBUG_MSG("NetworkProcessor EndRun()");
}

void RemoteDesktop::NetworkProcessor::Receive(std::shared_ptr<SocketHandler>& h){
	h->Receive();
	_Queue.push(h);
}