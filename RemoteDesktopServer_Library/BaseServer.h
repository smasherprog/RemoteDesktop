#ifndef BASESERVER_H
#define BASESERVER_H
#include "CommonNetwork.h"
#include <thread>
#include <memory>
#include <mutex>
#include "..\RemoteDesktop_Library\Delegate.h"


namespace RemoteDesktop{
	class SocketHandler;
	class DesktopMonitor;
	class BaseServer {

		void _OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s);
		void _OnConnectHandler(SocketHandler* socket);
		void _OnDisconnectHandler(SocketHandler* socket);

		void _OnDisconnect(int index);
		void _OnConnect(SOCKET listensocket);
		bool _Listen(unsigned short port);
		void _ConnectWrapper(unsigned short port, std::wstring host);
		void _RunReverse(SOCKET sock, std::wstring aes);

		void _CleanupSockets_and_Events();

		void _HandleDisconnects_DesktopSwitches();

		void _ListenWrapper(unsigned short port, std::wstring host);
		
		std::mutex _DisconectClientLock;
		std::vector<SocketHandler*> _DisconectClientList;
		std::unique_ptr<DesktopMonitor> _DesktopMonitor;

		std::mutex _SocketArrayLock;
		std::thread _BackGroundNetworkWorker;
		std::vector<WSAEVENT> EventArray;

		
	protected:
		int MaxConnectAttempts = 3;
		
		std::vector<std::shared_ptr<SocketHandler>> SocketArray;
		bool Running = false;
		bool ReverseConnection = false;
			bool DisconnectReceived = false;

		Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> Receive_CallBack;
		Delegate<void, std::shared_ptr<SocketHandler>&> Connected_CallBack;
		Delegate<void, std::shared_ptr<SocketHandler>&> Disconnect_CallBack;

	public:
		BaseServer(Delegate<void, std::shared_ptr<SocketHandler>&> c,
			Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> r,
			Delegate<void, std::shared_ptr<SocketHandler>&> d);
		~BaseServer();

		bool Is_Running() const{ return Running; }

		size_t Client_Count() const { return  SocketArray.empty() ? 0 : (ReverseConnection ? SocketArray.size() : SocketArray.size() - 1); }
		void StartListening(unsigned short port, std::wstring host);
		void ForceStop();
		void GracefulStop(){ DisconnectReceived = true; Running = false; }
		
		void SendToAll(NetworkMessages m, NetworkMsg& msg );
		Proxy_Header ProxyHeader;
	};

};

#endif