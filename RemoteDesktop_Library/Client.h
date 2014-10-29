#ifndef CLIENT_H
#define CLIENT_H
#include "BaseClient.h"

namespace RemoteDesktop{

	class Client : public BaseClient{


	public:
		Client();
		virtual ~Client() override;
		virtual void OnDisconnect(SocketHandler& sh) override;
		virtual void OnConnect(SocketHandler& sh) override;
		virtual void OnReceive(SocketHandler& sh)  override;

	};

};


#endif