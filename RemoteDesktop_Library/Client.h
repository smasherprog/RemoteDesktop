#ifndef CLIENT_H
#define CLIENT_H
#include "BaseClient.h"

namespace RemoteDesktop{
#if defined _DEBUG
	class CConsole;
	class ImageCompression;
#endif

	class Client : public BaseClient{
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::unique_ptr<ImageCompression> _ImageCompression;
	
	public:
		Client();
		virtual ~Client() override;
		virtual void OnDisconnect(SocketHandler& sh) override;
		virtual void OnConnect(SocketHandler& sh) override;
		virtual void OnReceive(SocketHandler& sh)  override;
		HWND _HWND;
	};

};


#endif