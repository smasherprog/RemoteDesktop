#ifndef NETWORK_GATEWAYSERVER123_H
#define NETWORK_GATEWAYSERVER123_H
#include <string>
#include <thread>
#include <memory>

namespace RemoteDesktop{
	class Gateway_Socket;
	class GatewayServer{
		std::thread _BackgroundWorker;		
		std::wstring _Host, _Port;
		void _Run();
		bool _Running = false; 
		
		void _HandleConnect(std::shared_ptr<Gateway_Socket>& ptr);
		void _HandleDisconnect(std::shared_ptr<Gateway_Socket>& ptr);

		void(__stdcall * _OnConnect)();
		void(__stdcall * _OnDisconnect)();
		void _CheckForDisconnects(std::vector<void*>& eventarray, std::vector<std::shared_ptr<Gateway_Socket>>& socketarray);

	public:
		GatewayServer(void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)());
		~GatewayServer();

		void Start(std::wstring port, std::wstring host);
		void Stop(bool blocking = false);
	};
}

#endif