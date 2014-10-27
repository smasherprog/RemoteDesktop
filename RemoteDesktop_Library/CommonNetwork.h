#ifndef COMMONNETWORK_H
#define COMMONNETWORK_H


typedef void(__stdcall *OnConnectCB)();
namespace RemoteDesktop{
	#define NETWORKHEADERSIZE sizeof(int) + sizeof(unsigned char)
	enum NetworkMessages{
		INVALID=-1,
		RESOLUTIONCHANGE,
		UPDATEREGION,
		MOUSEEVENT,
		MOUSEIMAGEEVENT,
		PING,
		KEYEVENT,
		FOLDER,
		FILE
	};

}


#endif