#ifndef SERVER_H
#define SERVER_H
#include <memory>
#include <mutex>
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include <thread>


namespace RemoteDesktop{

	class MouseCapture;
	class DesktopMonitor;
	class BaseServer;
	class SocketHandler;
	struct Packet_Header;
	class Image;
	class Rect;
	class ClipboardMonitor;
	struct Clipboard_Data;
	class SystemTray;

#if _DEBUG
	class CConsole;
#endif
	class RD_Server{
#if _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::vector<std::shared_ptr<SocketHandler>> _NewClients;
	
		std::unique_ptr<MouseCapture> mousecapturing;
		std::unique_ptr<DesktopMonitor> _DesktopMonitor;
		std::unique_ptr<BaseServer> _NetworkServer;
		
		std::unique_ptr<ClipboardMonitor> _ClipboardMonitor;
		std::unique_ptr<SystemTray> _SystemTray;

		std::mutex _NewClientLock;

		void _HandleNewClients(Image& img);
		bool _HandleResolutionUpdates(Image& img, Image& _lastimg);
		void _Handle_ScreenUpdates(Image& img, Rect& rect);
		void _Handle_MouseUpdates(const std::unique_ptr<MouseCapture>& mousecapturing);

		void _Handle_MouseUpdate(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		void _Handle_File(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh); 
		void _Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_DisconnectandRemove(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_Settings(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ConnectionInfo(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);

		void _OnClipboardChanged(const Clipboard_Data& c);
		void _CreateSystemMenu();
		void _TriggerShutDown();


		void _TriggerShutDown_and_RemoveSelf();

		bool _RunningAsService = false;
		bool _RunningReverseProxy = false;

		RAIIHANDLE_TYPE _CADEventHandle;
		RAIIHANDLE_TYPE _SelfRemoveEventHandle;

		bool _RemoveOnExit = false;
		bool GetProxyID(std::wstring url, std::wstring& aeskey);

	public:
		RD_Server();
		~RD_Server();

		void OnDisconnect(std::shared_ptr<SocketHandler>& sh);
		void OnConnect(std::shared_ptr<SocketHandler>&  sh);
		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>&  sh) ;
		void Listen(unsigned short port, std::wstring host = L"", bool reverseconnecttoproxy=false);

	
	};

};


#endif