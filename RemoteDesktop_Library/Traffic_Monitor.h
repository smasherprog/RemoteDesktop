#ifndef TRAFFIC_MONITOR_H
#define TRAFFIC_MONITOR_H
#include <chrono>
#include <vector>
#include "CommonNetwork.h"

namespace RemoteDesktop{

	class Traffic_Monitor{		
		std::chrono::high_resolution_clock::time_point _SendTimer, _RecvTimer;

		std::vector<long long> _UncompressedSendBuffer, _UncompressedRecvBuffer;
		std::vector<long long> _CompressedSendBuffer, _CompressedRecvBuffer;
		Traffic_Stats _Stats;
	public:
		Traffic_Monitor();

		void UpdateSend(long uncompressed, long compressed);
		void UpdateRecv(long uncompressed, long compressed);

		const Traffic_Stats& get_TrafficStats() const{ 
			return _Stats; 
		}

	};
};


#endif
