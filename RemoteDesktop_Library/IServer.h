#ifndef ISERVER_H
#define ISERVER_H
#include "SocketHandler.h"
#include "CommonNetwork.h"

namespace RemoteDesktop{
	class IServer{
	public:
		virtual ~IServer(){}
		virtual void Listen(unsigned short port) = 0;
		virtual void OnConnect() = 0;
		virtual void OnConnect(OnConnectCB onconnectcb) = 0;
		virtual void OnReceive(SocketHandler sh) = 0;
		virtual void Stop() = 0;
	};

};

#endif
