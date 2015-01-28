#ifndef SERVER_H
#define SERVER_H
#include <memory>
#include <mutex>
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include <thread>


namespace RemoteDesktop{

	class MouseCapture;
	class DesktopMonitor;

	class SocketHandler;
	struct Packet_Header;
	class Rect;
	class ClipboardMonitor;
	struct Clipboard_Data;
	class SystemTray;
	class GatewayConnect_Dialog;
	class NewConnect_Dialog;
	class INetwork;
	class Screen;
	class DesktopBackground;

	class Server{

		std::mutex _ClientLock;
		std::vector<std::shared_ptr<SocketHandler>> _PendingNewClients; 
		std::vector<std::shared_ptr<SocketHandler>> _NewClients;
		std::unique_ptr<DesktopMonitor> _DesktopMonitor;
		std::shared_ptr<INetwork> _NetworkServer;
		
		std::unique_ptr<ClipboardMonitor> _ClipboardMonitor;
		std::unique_ptr<SystemTray> _SystemTray;
		std::unique_ptr<DesktopBackground> _DesktopBackground;

		void _HandleNewClients(Screen& screen, std::vector<std::shared_ptr<SocketHandler>>& newclients);
		void _HandleResolutionChanged(const Screen& screen);
		void _Handle_ScreenChanged(const Screen& img, const Rect& rect);

		void _Handle_MouseChanged(const MouseCapture& mousecapturing);
		void _Handle_UAC_Permission();

		void _Handle_MouseUpdate(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_File(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh); 	
		void _Handle_File_Flush(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_DisconnectandRemove(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_Settings(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ConnectionRequest(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ElevateProcess(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);

		void _OnClipboardChanged(const Clipboard_Data& c);
		void _CreateSystemMenu();
		void _TriggerShutDown();

		void _OnAllowConnection(std::wstring name);
		void _OnDenyConnection(std::wstring name);

		void _ShowGatewayDialog(int id);

		void _TriggerShutDown_and_RemoveSelf();

		bool _RunningAsService = false;
		bool _RunningReverseProxy = false;

		RAIIHANDLE_TYPE _CADEventHandle;
		RAIIHANDLE_TYPE _SelfRemoveEventHandle;

		std::shared_ptr<GatewayConnect_Dialog> _GatewayConnect_Dialog;
		std::shared_ptr<NewConnect_Dialog> _NewConnect_Dialog;

		bool _RemoveOnExit = false;
		bool GetProxyID(std::wstring url, std::wstring& aeskey);
		void _Run();

		
	public:
		Server();
		~Server();

		void OnDisconnect(std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void OnConnect(std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void Listen(std::wstring port, std::wstring host = L"");
		void ReverseConnect(std::wstring port, std::wstring host, std::wstring gatewayurl);
	
	};

};


#endif