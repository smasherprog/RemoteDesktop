#ifndef SERVER_H
#define SERVER_H
#include "BaseServer.h"


namespace RemoteDesktop{
	class Server : public BaseServer{

	public:
		Server();
		virtual ~Server();

		virtual void OnConnect() override;
		virtual void OnReceive(SocketHandler sh)  override;

	};

};


#endif