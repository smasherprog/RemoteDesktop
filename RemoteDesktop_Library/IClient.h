#ifndef ICLIENT_H
#define ICLIENT_H
#include "SocketHandler.h"
#include "CommonNetwork.h"

namespace RemoteDesktop{
	class IClient{
	public:
		virtual ~IClient(){}

		virtual void Connect(const char* host, const char*port) = 0;
		virtual void OnConnect(SocketHandler& sh) = 0;
		virtual void OnDisconnect(SocketHandler& sh) = 0;
		virtual void OnReceive(SocketHandler& sh) = 0;
		virtual void Send(SOCKET s, NetworkMessages m, NetworkMsg& msg) = 0;

		virtual void Stop() = 0;
	};

};

#endif
