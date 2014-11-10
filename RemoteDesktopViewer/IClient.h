#ifndef ICLIENT_H
#define ICLIENT_H
#include "SocketHandler.h"
#include "CommonNetwork.h"

namespace RemoteDesktop{
	class IClient{
	public:
		virtual ~IClient(){}
		//network stuff
		virtual void Connect(const char* host, const char*port) = 0;
		virtual void OnConnect(SocketHandler& sh) = 0;
		virtual void OnDisconnect(SocketHandler& sh) = 0;
		virtual void OnReceive(SocketHandler& sh) = 0;
		virtual int Send(NetworkMessages m, NetworkMsg& msg) = 0;
		virtual void Stop() = 0;

		//desktop stuff
		virtual void Draw(HDC hdc) = 0;
		virtual void KeyEvent(int VK, bool down) = 0;
		virtual void MouseEvent(unsigned int action, int x, int y, int wheel) = 0;
		virtual bool SetCursor() = 0;
		
	};

};

#endif
