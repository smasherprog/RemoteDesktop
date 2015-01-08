#include "stdafx.h"
#include "NetworkProcessor.h"
#include "SocketHandler.h"
#include "Desktop_Monitor.h"

RemoteDesktop::NetworkProcessor::NetworkProcessor(Delegate<void, std::vector<char>&, int> d) {
	_Receive_CallBack = d;
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
bool RemoteDesktop::NetworkProcessor::Add(std::vector<char>& buffer, int counter){
	DEBUG_MSG("Add %", counter);
	int timewait = 0;
	while (_Count>1024*1024*30){//dont exceed 30 MB....
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		_CV.notify_one();//try to wake the other thread up to do work...
		timewait += 5;
		if (timewait > 1000 * 60 || !_Running){// thread has waited for over a minute... Disconnect
			return false;
		}
	}
	if (!_Running) return false;
	std::lock_guard<std::mutex> lock(_ContainerLock);
	_ReceiveBuffer_In.resize(_Count + counter);
	memcpy(_ReceiveBuffer_In.data() + _Count, buffer.data(), counter);
	_Count += counter;
	DEBUG_MSG("Waking other thread up");
	_CV.notify_one();
	return true;
}
void RemoteDesktop::NetworkProcessor::_Run(){
	while (_Running){
		std::unique_lock<std::mutex> lk(_Lock);
		_CV.wait(lk);
		DEBUG_MSG("_RUn Woke up!");
		if (!_DesktopMonitor->Is_InputDesktopSelected()) _DesktopMonitor->Switch_to_Desktop(DesktopMonitor::Desktops::INPUT);
		if (!_Running) return;
		int count = 0;
		{
			std::lock_guard<std::mutex> lock(_ContainerLock);
			count = _Count;
			_Count = 0;
			_ReceiveBuffer_Out.resize(count);
			memcpy(_ReceiveBuffer_Out.data(), _ReceiveBuffer_In.data(), count);
		}
		DEBUG_MSG("Processing % ", count);
		if (!_Running) return;
		_Receive_CallBack(_ReceiveBuffer_Out, count);
	}

}