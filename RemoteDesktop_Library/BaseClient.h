#ifndef BASECLIENT_H
#define BASECLIENT_H
#include "IClient.h"
#include "SocketHandler.h"
#include "CommonNetwork.h"
#include "SocketHandler.h"
#include <thread>

namespace RemoteDesktop{
	class BaseClient : public IClient{
		bool _Connect(const char* host, const char* port);
		void _OnReceive(SocketHandler& sh);

		bool Running = false;
		SocketHandler _Socket;

		void _Run();
		std::thread _BackGroundNetworkWorker;

	public:
		BaseClient();
		virtual ~BaseClient() override;
		virtual void Connect(const char* host, const char* port="443") override;

		virtual void Send(SOCKET s, NetworkMessages m, NetworkMsg& msg) override;
		virtual void Stop() override;
	};

};

#endif