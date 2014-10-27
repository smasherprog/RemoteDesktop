#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <vector>
#include "CommonNetwork.h"

namespace RemoteDesktop{
	struct SocketHandler{
		SOCKET socket;
		sockaddr_in addr;
		std::vector<char> Buffer;
		int bytecounter = 0;
		int msglength = 0;
		NetworkMessages msgtype = NetworkMessages::INVALID;
	};
};

#endif