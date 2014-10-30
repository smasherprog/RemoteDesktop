#ifndef COMMONNETWORK_H
#define COMMONNETWORK_H


typedef void(__stdcall *OnConnectCB)();
namespace RemoteDesktop{
#define NETWORKHEADERSIZE sizeof(int) + sizeof(unsigned char)
	enum NetworkMessages{
		INVALID = -1,
		RESOLUTIONCHANGE,
		UPDATEREGION,
		MOUSEEVENT,
		MOUSEIMAGEEVENT,
		PING,
		KEYEVENT,
		FOLDER,
		FILE
	};
	class NetworkMsg;
	class SocketHandler;
	namespace _INTERNAL{
		void _Send(SOCKET s, NetworkMessages m, NetworkMsg& msg);
		void _SendLoop(SOCKET s, char* data, int len);
		int _ProcessPacketHeader(RemoteDesktop::SocketHandler& sh);
		int _ProcessPacketBody(RemoteDesktop::SocketHandler& sh);
		void _RecevieEnd(RemoteDesktop::SocketHandler& sh);
	};

}


#endif