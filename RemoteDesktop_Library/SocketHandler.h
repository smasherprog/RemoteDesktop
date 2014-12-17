#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

#include "CommonNetwork.h"
#include <memory>

#include "Encryption.h"
#include <vector>
#include "Traffic_Monitor.h"
#include <mutex>
#include "Delegate.h"

namespace RemoteDesktop{
	enum PeerState{
		PEER_STATE_DISCONNECTED,
		PEER_STATE_CONNECTING,
		PEER_STATE_CONNECTED,
		PEER_STATE_EXCHANGING_KEYS,
	};
	class socket_wrapper{
	public:
		SOCKET socket;

		explicit socket_wrapper(SOCKET s) : socket(s) { }
		~socket_wrapper();
		
	};
	class SocketHandler{
		std::mutex _SendLock;
		std::vector<char> _ReceivedBuffer, _SendBuffer;
		std::vector<char> _ReceivedCompressionBuffer, _SendCompressionBuffer;

		int _ReceivedBufferCounter = 0;
		int _ReceiveCounter = 0;
		int _SendCounter = 0;
		Network_Return _SendLoop(char* data, int len);
		Network_Return _ReceiveLoop();

		Packet_Encrypt_Header _Encypt_Header;
		Encryption _Encyption;

		Network_Return _Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg); 
		Network_Return _Decrypt_Received_Data();
		Network_Return _Complete_Key_Exchange();
		
		std::unique_ptr<socket_wrapper> _Socket;
		Network_Return _Disconnect();
		
		

	public:
		explicit SocketHandler(SOCKET socket, bool client);

		Network_Return Exchange_Keys(int id);
		PeerState State = PEER_STATE_DISCONNECTED;
		Network_Return Send(NetworkMessages m, const NetworkMsg& msg);
		Network_Return Receive();
		SOCKET get_Socket() const { return _Socket ? _Socket->socket : INVALID_SOCKET; }

		Delegate<void, Packet_Header*, const char*, SocketHandler*> Receive_CallBack;
		Delegate<void, SocketHandler*> Connected_CallBack;
		Delegate<void, SocketHandler*> Disconnect_CallBack;
	
		Traffic_Monitor Traffic;
		Network_Return CheckState();
		
		std::wstring UserName;
	};
};

#endif