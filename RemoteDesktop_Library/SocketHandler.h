#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include "CommonNetwork.h"
#include <memory>
#define STARTBUFFERSIZE 1024 *1024 *4
#include "Encryption.h"

namespace RemoteDesktop{
	class socket_wrapper{
	public:
		SOCKET socket;
		explicit socket_wrapper(SOCKET s) : socket(s) { }
		~socket_wrapper();
	};
	class SocketHandler{
	public:
		SocketHandler();
		Encryption _Encyption;
		std::shared_ptr<socket_wrapper> socket;
		sockaddr_in addr;
		std::vector<char> Buffer;
		int bytecounter = 0;
		int msglength = 0;
		NetworkMessages msgtype = NetworkMessages::INVALID;
		void clear();

	};
};

#endif