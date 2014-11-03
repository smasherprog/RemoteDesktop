#ifndef ISERVER_H
#define ISERVER_H
#include "SocketHandler.h"
#include "CommonNetwork.h"

namespace RemoteDesktop{
	class IServer{
	public:
		virtual ~IServer(){}
		virtual void Listen(unsigned short port) = 0;
		virtual void OnConnect(SocketHandler& sh) = 0;
		virtual void OnDisconnect(SocketHandler& sh) = 0;
		virtual void SetOnConnectCallback(OnConnectCB onconnectcb) = 0;
		virtual void OnReceive(SocketHandler& sh) = 0;

		virtual int Send(SOCKET s, NetworkMessages m, NetworkMsg& msg) = 0;
		virtual void SendToAll(NetworkMessages m, NetworkMsg& msgs) = 0;

		virtual void Stop() = 0;
	};

};

#endif
