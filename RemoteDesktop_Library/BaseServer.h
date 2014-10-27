#ifndef BASESERVER_H
#define BASESERVER_H
#include "IServer.h"
#include "SocketHandler.h"
#include "CommonNetwork.h"
#include <thread>

namespace RemoteDesktop{
	class BaseServer : public IServer{
		void Reset();
		int _OnReceiveBegin(SocketHandler& sh);//this function is responsible for collecting a full message of data
		void _OnReceiveEnd(SocketHandler& sh);//after OnReceive is called, this is called to do any neccessary cleanup

		int _ProcessPacketHeader(SocketHandler& sh);
		int _ProcessPacketBody(SocketHandler& sh);
		int _OnConnect(SOCKET listensocket);
		void _Listen(unsigned short port);

		std::thread _BackGroundWorker;
		std::vector<WSAEVENT> EventArray;

	protected:
		std::vector<SocketHandler> SocketArray;
		bool Running = false;

		void(__stdcall *_OnConnectCB)() = nullptr;
	public:
		BaseServer();
		virtual ~BaseServer() override;
		
		virtual void Stop() override;
		virtual void OnConnect(OnConnectCB callback) override{ _OnConnectCB = callback; }
		virtual void OnConnect() override;
		virtual void OnReceive(SocketHandler sh) = 0;
		virtual void Listen(unsigned short port) override;	
	};

};

#endif