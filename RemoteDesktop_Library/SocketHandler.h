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
#include "Handle_Wrapper.h"
#include "NetworkProcessor.h"

namespace RemoteDesktop{
	enum PeerState{
		PEER_STATE_DISCONNECTED,
		PEER_STATE_CONNECTING,
		PEER_STATE_CONNECTED,
		PEER_STATE_EXCHANGING_KEYS,
		PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES
	};

	namespace INTERNAL{
		//improves speed when memory allocations are kept down because vector resize always does a memset on the unintialized elements
		extern std::vector<std::vector<char>> SocketBufferCache;
		extern std::mutex SocketBufferCacheLock;
	}
	class SocketHandler{
		
		std::mutex _SendLock;

		std::vector<char> _SendBuffer, _ReceivedBuffer, OtherReceiveBuffer;
		std::vector<char> _ReceivedCompressionBuffer, _SendCompressionBuffer;

		int _ReceivedBufferCounter = 0;

		Packet_Encrypt_Header _Encypt_Header;
		Encryption _Encyption;

		Network_Return _Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg); 
		Network_Return _Decrypt_Received_Data();
		Network_Return _Complete_Key_Exchange();
		
		RAIISOCKET_TYPE _Socket;
		
		NetworkProcessor _Processor;
		Network_Return _Disconnect();
		void _Receive(std::vector<char>& buffer, int count);

	public:
		explicit SocketHandler(SOCKET socket, bool client);
		~SocketHandler();
		Network_Return Exchange_Keys(int dst_id, int src_id, std::wstring aeskey);
		PeerState State = PEER_STATE_DISCONNECTED;
		Network_Return Send(NetworkMessages m, const NetworkMsg& msg); 
		Network_Return Send(NetworkMessages m);
		Network_Return Receive();
		SOCKET get_Socket() const { return _Socket ? _Socket->socket : INVALID_SOCKET; }

		//this is a graceful disconnect and will disconnect on the next loop iteration
		//will not call any disconnect callbacks until the disconnect actually occurs
		void Disconnect(){ State = PEER_STATE_DISCONNECTED; }

		bool Authorized = false;
		std::function<void(Packet_Header*, const char*, SocketHandler*)> Receive_CallBack;
		std::function<void(SocketHandler*)> Connected_CallBack;
		std::function<void(SocketHandler*)> Disconnect_CallBack;
	
		Traffic_Monitor Traffic;
		Network_Return CheckState();
		
		User_Info_Header Connection_Info;
	};
};

#endif