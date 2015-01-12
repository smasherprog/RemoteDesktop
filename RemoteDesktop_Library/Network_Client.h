#ifndef NETWORKCLIENT123_H
#define NETWORKCLIENT123_H
#include "INetwork.h"
#include <memory>
#include <thread>

namespace RemoteDesktop{
	class SocketHandler;

	class Network_Client : public INetwork{

		bool _ShouldDisconnect = false;
		void _Run_Standard(int dst_id, std::wstring aeskey);
		void _Run_Gateway(std::wstring gatewayurl);
		void _Run(std::shared_ptr<SocketHandler>& socket);
		std::wstring _Dst_Host, _Dst_Port;
		std::thread _BackgroundWorker;
		int MaxConnectAttempts = DEFAULTMAXCONNECTATTEMPTS;
		std::weak_ptr<SocketHandler> _Socket;//this is a weak ptr to denote ownership. The class does not own this, the function _Run does..
		void Setup(std::wstring port, std::wstring host);
		int _Connections = 0;

		void _HandleConnect(SocketHandler* ptr);
		void _HandleDisconnect(SocketHandler* ptr); 
		void _HandleGatewayDisconnect(SocketHandler* ptr);
		void _HandleReceive(Packet_Header* p, const char* d, SocketHandler* ptr);

	public:
		Network_Client();

		virtual ~Network_Client();	
		virtual void Start(std::wstring port, std::wstring host) override;
		void Start(std::wstring port, std::wstring host, int id, std::wstring aeskey);
		void Start(std::wstring port, std::wstring host, std::wstring gatewayurl);

		virtual void Stop(bool blocking = false)override;
		virtual void Set_RetryAttempts(int num_of_retry)override { MaxConnectAttempts = num_of_retry; }
		virtual int Get_RetryAttempts(int num_of_retry) const override { return MaxConnectAttempts; }
		virtual RemoteDesktop::Network_Return Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg) override;
		virtual int Connection_Count() const override { return _Connections; }

		std::function<void(int)> OnGatewayConnected;
	};

}

#endif