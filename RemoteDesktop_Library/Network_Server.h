#ifndef NETWORKSERVER123_H
#define NETWORKSERVER123_H
#include "INetwork.h"
#include <memory>
#include <thread>
#include <vector>
#include <mutex>

namespace RemoteDesktop{

	class SocketHandler;

	class Network_Server : public INetwork{

		void _Run();

		std::wstring _Host, _Port;
		std::thread _BackgroundWorker;
		int MaxConnectAttempts = DEFAULTMAXCONNECTATTEMPTS;

		std::vector<WSAEVENT> EventArray;
		std::vector<std::shared_ptr<SocketHandler>> SocketArray;

		void _CheckForDisconnects();
		void _HandleNewConnect(SOCKET sock);
	
		void _HandleConnect(SocketHandler* ptr);
		void _HandleDisconnect(SocketHandler* ptr);
		void _HandleReceive(Packet_Header* p, const char* d, SocketHandler* ptr);

	public:
		Network_Server();

		virtual ~Network_Server();	
		virtual void Start(std::wstring port, std::wstring host) override;
		virtual void Stop(bool blocking = false) override;
		virtual void Set_RetryAttempts(int num_of_retry)override { MaxConnectAttempts = num_of_retry; }
		virtual int Get_RetryAttempts(int num_of_retry) const override{ return MaxConnectAttempts; }
		virtual RemoteDesktop::Network_Return Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg, Auth_Types to_which_type) override;
		virtual int Connection_Count() const override { auto s = SocketArray.size(); if (s == 0) return 0; return s - 1; }
	};

}

#endif