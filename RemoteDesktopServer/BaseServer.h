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
		HDESK _LastNetworkCurrentDesktop = NULL;

	protected:
		std::vector<SocketHandler> SocketArray;
		bool Running = false;
		HDESK _NetworkCurrentDesktop = NULL;
		void StartListening(unsigned short port, HDESK h);
	public:
		BaseServer();
		virtual ~BaseServer() override;
		
		virtual void Stop() override;

		virtual int Send(SOCKET s, NetworkMessages m, NetworkMsg& msg)override;
		virtual void SendToAll(NetworkMessages m, NetworkMsg& msg )override;


	};

};

#endif