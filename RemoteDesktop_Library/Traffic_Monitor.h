#ifndef TRAFFIC_MONITOR_H
#define TRAFFIC_MONITOR_H
#include <chrono>
#include <vector>

namespace RemoteDesktop{

	class Traffic_Monitor{		
		std::chrono::high_resolution_clock::time_point _SendTimer, _RecvTimer;
		long _SendBytes, _RecvBytes;//overall lifetime totals 
		long _SendBPS, _RecvBPS;
		std::vector<long> _SendBuffer, _RecvBuffer;
	public:
		Traffic_Monitor();

		void UpdateSend(long b);
		void UpdateRecv(long b);

		long get_TotalSendBytes() const{ return _SendBytes; }
		long get_TotalRecvBytes() const{ return _RecvBytes; }
		long get_SendBPS() const{ return _SendBPS; }
		long get_RecvBPS() const{ return _RecvBPS; }
	};
};


#endif
