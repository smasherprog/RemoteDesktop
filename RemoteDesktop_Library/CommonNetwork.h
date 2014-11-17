#ifndef COMMONNETWORK_H
#define COMMONNETWORK_H
#include "Rect.h"
#include <vector>

#define IVSIZE 16

typedef void(__stdcall *OnConnectCB)();
namespace RemoteDesktop{
	//ensure ths is tighly packed
#pragma pack(push, 1)
	struct Packet_Header{
		int PayloadLen = 0;
		char Packet_Type = -1;
	};	
	struct Packet_Encrypt_Header{
		int PayloadLen = IVSIZE;//should include the IV size 
		char IV[IVSIZE];
	};
	struct Image_Diff_Header{
		Rect rect;
		char compressed = 0;
	};	
	struct KeyEvent_Header{
		int VK;
		char down = 0;
	};	
	struct MouseEvent_Header{
		Point pos;
		int HandleID;
		unsigned int Action;
		int wheel;
	};
#pragma pack(pop)
#define NETWORKHEADERSIZE sizeof(Packet_Encrypt_Header)
#define MAXMESSAGESIZE 1024*1024 *50  //50 MB is the largest single message that is allowed. This is to prevent crashing either the client or server by sending fake packet lengths
	enum NetworkMessages{
		INVALID = -1,
		RESOLUTIONCHANGE,
		UPDATEREGION,
		MOUSEEVENT,
		KEYEVENT,
		FOLDER,
		FILE,	
		CAD,
		INIT_ENCRYPTION
	};
	enum Network_Return{
		FAILED,
		COMPLETED,
		PARTIALLY_COMPLETED
	};
	struct DataPackage{
		DataPackage(char*d, int l) : data(d), len(l) {}
		char* data = nullptr;
		int len = 0;
	};
	class NetworkMsg{
	public:
		NetworkMsg(){}
		int payloadlength()const{ auto l = 0; for (auto& a : data) l += a.len; return l; }
		std::vector<DataPackage> data;
		template<class T>void push_back(const T& x){ data.push_back(DataPackage((char*)&x, sizeof(x))); }
	};


}


#endif