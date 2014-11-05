#ifndef BASESERVER_H
#define BASESERVER_H
#include "IServer.h"
#include "SocketHandler.h"
#include "CommonNetwork.h"
#include <thread>

namespace RemoteDesktop{
	class BaseServer : public IServer{

		void _OnReceive(SocketHandler& sh);//this function is responsible for collecting a full message of data
		void _OnDisconnect(int index);
		void _OnConnect(SOCKET listensocket);
		bool _Listen(unsigned short port);
		void _ListenWrapper(unsigned short port);

		std::thread _BackGroundNetworkWorker;
		std::vector<WSAEVENT> EventArray;

	protected:
		std::vector<SocketHandler> SocketArray;
		bool Running = false;

	public:
		BaseServer();
		virtual ~BaseServer() override;
		
		virtual void Stop() override;

		virtual void Listen(unsigned short port) override;	
		virtual int Send(SOCKET s, NetworkMessages m, NetworkMsg& msg)override;
		virtual void SendToAll(NetworkMessages m, NetworkMsg& msg )override;


	};

};

#endif