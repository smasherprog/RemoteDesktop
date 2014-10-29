#ifndef SERVER_H
#define SERVER_H
#include "BaseServer.h"
#include <memory>
#include "Image.h"
#include <mutex>

namespace RemoteDesktop{
	class ScreenCapture;
	class ImageCompression;
#if defined _DEBUG
	class CConsole;
#endif
	class Server : public BaseServer{
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::vector<SocketHandler> _NewClients;
		std::mutex _NewClientLock;

		void _Run();
		std::thread _BackGroundCapturingWorker;
		void _HandleNewClients(NetworkMsg& msg, std::vector<SocketHandler>& newclients, Image& img, const std::unique_ptr<ImageCompression>& imagecompression);

	public:
		Server();
		virtual ~Server() override;
		virtual void Stop() override;
		virtual void OnDisconnect(SocketHandler& sh) override;
		virtual void OnConnect(SocketHandler& sh) override;
		virtual void OnReceive(SocketHandler& sh)  override;
		virtual void Listen(unsigned short port) override;
	};

};


#endif