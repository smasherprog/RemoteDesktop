#ifndef COMMONNETWORK_H
#define COMMONNETWORK_H
#include "Rect.h"

typedef void(__stdcall *OnConnectCB)();
namespace RemoteDesktop{
	//ensure ths is tighly packed
#pragma pack(push, 1)
	struct Packet_Header{
		int PayloadLen = 0;
		char Packet_Type = -1;
	};
	struct Image_Diff_Header{
		Rect rect;
		char compressed = 0;
	};	
	struct KeyEvent_Header{
		int VK;
		char down = 0;
	};
#pragma pack(pop)
#define NETWORKHEADERSIZE sizeof(Packet_Header)
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
		int _Send(SOCKET s, NetworkMessages m, NetworkMsg& msg);
		int _SendLoop(SOCKET s, char* data, int len);
		int _ProcessPacketHeader(RemoteDesktop::SocketHandler& sh);
		int _ProcessPacketBody(RemoteDesktop::SocketHandler& sh);
		void _RecevieEnd(RemoteDesktop::SocketHandler& sh);
	};

}


#endif