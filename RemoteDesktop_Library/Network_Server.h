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
		//weak ptrs are not expensive.. a single atomic operation is all it takes to convert to shared_ptr, but this ensures the lifetime of the sockets is managed correctly.
		std::weak_ptr<std::vector<std::shared_ptr<SocketHandler>>> _Sockets;
		void _CheckForDisconnects(std::vector<WSAEVENT>& eventarray, std::vector<std::shared_ptr<SocketHandler>>& socketarray);
		void _HandleNewConnect(SOCKET sock, std::vector<WSAEVENT>& EventArray, std::vector<std::shared_ptr<SocketHandler>>& SocketArray);
	
		void _HandleConnect(std::shared_ptr<SocketHandler>& ptr);
		void _HandleDisconnect(std::shared_ptr<SocketHandler>& ptr);
		void _HandleReceive(Packet_Header* p, const char* d, std::shared_ptr<SocketHandler>& ptr);

	public:
		Network_Server();

		virtual ~Network_Server();	
		virtual void Start(std::wstring port, std::wstring host) override;
		virtual void Stop(bool blocking = false) override;
		virtual void Set_RetryAttempts(int num_of_retry)override { MaxConnectAttempts = num_of_retry; }
		virtual int Get_RetryAttempts(int num_of_retry) const override{ return MaxConnectAttempts; }

		virtual RemoteDesktop::Network_Return Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg, Auth_Types to_which_type) override;

		virtual int Connection_Count() const override { 
			auto s(_Sockets.lock()); 
			if (!s) return 0; 
			return s->size() <= 1 ? 0 : s->size();
		}
	};

}

#endif