#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <vector>
#include "CommonNetwork.h"
#include <memory>
#define STARTBUFFERSIZE 1024 *1024 *4

namespace RemoteDesktop{
	class socket_wrapper{
	public:
		SOCKET socket;
		explicit socket_wrapper(SOCKET s) : socket(s) { }
		~socket_wrapper()
		{
			if (socket != INVALID_SOCKET){
				shutdown(socket, SD_RECEIVE);
				closesocket(socket);
			}
		}
	};
	class SocketHandler{
	public:
		SocketHandler(){
			Buffer.reserve(STARTBUFFERSIZE);
		}
		std::shared_ptr<socket_wrapper> socket;
		sockaddr_in addr;
		std::vector<char> Buffer;
		int bytecounter = 0;
		int msglength = 0;
		NetworkMessages msgtype = NetworkMessages::INVALID;
	};
	class NetworkMsg{
	public: 
		NetworkMsg(){}
		int payloadlength()const{ auto l = 0; for (auto& a : lens) l += a; return l; }
		std::vector<char*> data;
		std::vector<int> lens;
	};
};

#endif