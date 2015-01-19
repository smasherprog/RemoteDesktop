#ifndef GATEWAY_SOCKET123_H
#define GATEWAY_SOCKET123_H
#include "Handle_Wrapper.h"
#include "CommonNetwork.h"

namespace RemoteDesktop{
	class Gateway_Socket{

		RAIISOCKET_TYPE _Socket;
		std::vector<char> _Buffer;
		int _BufferCount = 0;
		PeerState State = PEER_STATE_DISCONNECTED;

	public:
		enum ConnectionTypes { VIEWER, SERVER };
		explicit Gateway_Socket(SOCKET socket) : _Socket(RAIISOCKET(socket)){}
		void Receive();

		Network_Return Disconnect(){ State = PEER_STATE_DISCONNECTED; return Network_Return::FAILED; }

		int Dst_ID = -1;
		int Src_ID = -1;

		RAIISOCKET_TYPE ThisSocket;
		std::weak_ptr<Gateway_Socket> Paired_Socket;
	};
}

#endif