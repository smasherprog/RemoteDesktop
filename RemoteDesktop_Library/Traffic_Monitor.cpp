#include "stdafx.h"
#include "Traffic_Monitor.h"

RemoteDesktop::Traffic_Monitor::Traffic_Monitor(){
	_SendTimer = std::chrono::high_resolution_clock::now();
	_RecvTimer = std::chrono::high_resolution_clock::now();
	_SendBytes = _RecvBytes = _SendBPS = _RecvBPS = 0;

}


void RemoteDesktop::Traffic_Monitor::UpdateSend(long b){
	_SendBytes += b;

	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _SendTimer).count() > 3000){
		long total = 0;
		for (auto &a : _SendBuffer) total += a;
		_SendBuffer.clear();
		if (total > 0) _SendBPS = total / 3;
		_SendTimer = std::chrono::high_resolution_clock::now(); 
	}
	else _SendBuffer.push_back(b);
}
void RemoteDesktop::Traffic_Monitor::UpdateRecv(long b){
	_RecvBytes += b;
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _RecvTimer).count() > 3000){
		long total = 0;
		for (auto &a : _RecvBuffer) total += a;
		_RecvBuffer.clear();
		if (total > 0) _RecvBPS = total / 3;
		_RecvTimer = std::chrono::high_resolution_clock::now();
	}
	else _RecvBuffer.push_back(b);
}