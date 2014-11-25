#ifndef SERVER_H
#define SERVER_H
#include <memory>
#include <mutex>

namespace RemoteDesktop{
	class ScreenCapture;
	class ImageCompression;
	class MouseCapture;
	class DesktopMonitor;
	class BaseServer;
	class SocketHandler;
	struct Packet_Header;
	class Image;
	class Rect;
#if defined _DEBUG
	class CConsole;
#endif
	class RD_Server{
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::vector<std::shared_ptr<SocketHandler>> _NewClients;
		std::unique_ptr<ImageCompression> imagecompression;
		std::unique_ptr<MouseCapture> mousecapturing;
		std::unique_ptr<DesktopMonitor> _DesktopMonitor;
		std::unique_ptr<BaseServer> _NetworkServer;
		std::unique_ptr<ScreenCapture> _ScreenCapture;

		std::mutex _NewClientLock;

		void _HandleNewClients(Image& img);
		bool _HandleResolutionUpdates(Image& img, Image& _lastimg);
		void _Handle_ScreenUpdates(Image& img, Rect& rect, std::vector<unsigned char>& buffer);
		void _Handle_MouseUpdates(const std::unique_ptr<MouseCapture>& mousecapturing);

		void _Handle_MouseUpdate(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		void _Handle_File(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh); 
		void _Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);

		bool _RunningAsService = false;

	public:
		RD_Server();
		~RD_Server();

		void OnDisconnect(std::shared_ptr<SocketHandler>& sh);
		void OnConnect(std::shared_ptr<SocketHandler>&  sh);
		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>&  sh) ;
		void Listen(unsigned short port);
	};

};


#endif