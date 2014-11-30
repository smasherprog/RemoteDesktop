#include "stdafx.h"
#include "Traffic_Monitor.h"

RemoteDesktop::Traffic_Monitor::Traffic_Monitor(){
	_SendTimer = std::chrono::high_resolution_clock::now();
	_RecvTimer = std::chrono::high_resolution_clock::now();
	memset(&_Stats, 0, sizeof(_Stats));

}


void RemoteDesktop::Traffic_Monitor::UpdateSend(long uncompressed, long compressed){

	_Stats.CompressedSendBytes += compressed;
	_Stats.UncompressedSendBytes += uncompressed;

	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _SendTimer).count() > 3000){
		long long total = 0;
		for (auto &a : _UncompressedSendBuffer) total += a;
		_UncompressedSendBuffer.clear();
		if (total > 0) _Stats.UncompressedSendBytes = total / 3;

		total = 0;
		for (auto &a : _CompressedSendBuffer) total += a;
		_CompressedSendBuffer.clear();
		if (total > 0) _Stats.CompressedSendBytes = total / 3;

		_SendTimer = std::chrono::high_resolution_clock::now(); 
	
	}
	else {
		_UncompressedSendBuffer.push_back(uncompressed);
		_CompressedSendBuffer.push_back(compressed);
	}
}
void RemoteDesktop::Traffic_Monitor::UpdateRecv(long uncompressed, long compressed){

	_Stats.CompressedRecvBytes += compressed;
	_Stats.UncompressedRecvBytes += uncompressed;

	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - _RecvTimer).count() > 3000){
		long long  total = 0;
		for (auto &a : _UncompressedRecvBuffer) total += a;
		_UncompressedRecvBuffer.clear();
		if (total > 0) _Stats.UncompressedRecvBPS = total / 3;

		total = 0;
		for (auto &a : _CompressedRecvBuffer) total += a;
		_CompressedRecvBuffer.clear();
		if (total > 0) _Stats.CompressedRecvBPS = total / 3;

		_RecvTimer = std::chrono::high_resolution_clock::now();
	}
	else {
		_UncompressedRecvBuffer.push_back(uncompressed); 
		_CompressedRecvBuffer.push_back(compressed);
	}
}