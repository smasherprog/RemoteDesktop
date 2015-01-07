#include "stdafx.h"
#include "NetworkProcessor.h"
#include "SocketHandler.h"
#include "Desktop_Monitor.h"

RemoteDesktop::NetworkProcessor::NetworkProcessor(){
	_Running = true;
	_DesktopMonitor = std::make_unique<DesktopMonitor>();
	_Worker = std::thread(&RemoteDesktop::NetworkProcessor::_Run, this);
}
RemoteDesktop::NetworkProcessor::~NetworkProcessor(){
	_Running = false;
	_CV.notify_all();
	if (std::this_thread::get_id() != _Worker.get_id()){
		if (_Worker.joinable()) _Worker.join();
	}
}

void RemoteDesktop::NetworkProcessor::ReceiveEvent(std::shared_ptr<SocketHandler>& s){
	std::lock_guard<std::mutex> lock(_ContainerLock);
	_Receives.emplace_back(s);
	_CV.notify_one();
}
void RemoteDesktop::NetworkProcessor::_Run(){
	while (_Running){
		std::unique_lock<std::mutex> lk(_Lock);
		_CV.wait(lk);
		if (!_DesktopMonitor->Is_InputDesktopSelected()) _DesktopMonitor->Switch_to_Desktop(DesktopMonitor::Desktops::INPUT);
		if (!_Running) return;
		std::vector<std::shared_ptr<SocketHandler>> r;
		{
			std::lock_guard<std::mutex> lock(_ContainerLock);
			r = _Receives;
			_Receives.resize(0);
		}
		for (auto& a : r){
			if (!_Running) return;
			a->Receive();
		}
	}

}