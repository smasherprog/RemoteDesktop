#ifndef BASESERVER_H
#define BASESERVER_H
#include "CommonNetwork.h"
#include <thread>
#include <memory>
#include <mutex>

namespace RemoteDesktop{
	class SocketHandler;
	class BaseServer {

		void _OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s);
		void _OnConnectHandler(SocketHandler* socket);
		void _OnDisconnectHandler(SocketHandler* socket);

		void _OnDisconnect(int index);
		void _OnConnect(SOCKET listensocket);
		bool _Listen(unsigned short port);
		void _ListenWrapper(unsigned short port);
		
		std::mutex _DisconectClientLock;
		std::vector<SocketHandler*> _DisconectClientList;

		std::mutex _SocketArrayLock;
		std::thread _BackGroundNetworkWorker;
		std::vector<WSAEVENT> EventArray;

	protected:
		std::vector<std::shared_ptr<SocketHandler>> SocketArray;
		bool Running = false;
		HDESK _NetworkCurrentDesktop = NULL;
		
		std::function<void(Packet_Header*, const char*, std::shared_ptr<SocketHandler>&)> Receive_CallBack;
		std::function<void(std::shared_ptr<SocketHandler>&)> Connected_CallBack;
		std::function<void(std::shared_ptr<SocketHandler>&)> Disconnect_CallBack;

	public:
		BaseServer(std::function<void(std::shared_ptr<SocketHandler>&)> c,
			std::function<void(Packet_Header*, const char*, std::shared_ptr<SocketHandler>&)> r,
			std::function<void(std::shared_ptr<SocketHandler>&)> d);
		~BaseServer();

		bool Is_Running() const{ return Running; }

		void SetThreadDesktop(HDESK h){ _NetworkCurrentDesktop = h; }
		size_t Client_Count() const { return  SocketArray.empty() ? 0 : SocketArray.size() -1; }
		void StartListening(unsigned short port, HDESK h);
		void Stop();
		void SendToAll(NetworkMessages m, NetworkMsg& msg );

	

	};

};

#endif