#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H
#include "CommonNetwork.h"
#include <memory>

#include "Encryption.h"
#include <vector>
#include "Traffic_Monitor.h"
#include <mutex>
#include "Handle_Wrapper.h"
#include "Delegate.h"


namespace RemoteDesktop{

	namespace INTERNAL{
		//improves speed when memory allocations are kept down because vector resize always does a memset on the unintialized elements
		extern std::vector<std::vector<char>> SocketBufferCache;
		extern std::mutex SocketBufferCacheLock;
	}
	class SocketHandler{
		
		std::mutex _SendLock, _ReceiveLock;
		std::vector<char> _SendBuffer, _ReceivedBuffer, _In_ReceivedBuffer;
		std::vector<char> _ReceivedCompressionBuffer, _SendCompressionBuffer;
		int _ReceivedBufferCounter = 0;
		int _In_ReceivedBufferCounter = 0;

		Packet_Encrypt_Header _Encypt_Header;
		Encryption _Encyption;

		Network_Return _Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg); 
		RAIISOCKET_TYPE _Socket;
		PeerState State = PEER_STATE_DISCONNECTED;
		std::unique_ptr<std::ofstream> _File;
		std::string _FileName;

	public:
		explicit SocketHandler(SOCKET socket, bool client);
		~SocketHandler();

		//used to keep a file open for writing
		void WriteToFile(std::string filename, const char* data, int len_bytes, bool lastwrite=false);
	
		Network_Return Exchange_Keys(int dst_id, int src_id, std::wstring aeskey);

		void Receive();
		Network_Return Send(NetworkMessages m, const NetworkMsg& msg); 
		Network_Return Send(NetworkMessages m);

		SOCKET get_Socket() const { return _Socket ? _Socket->socket : INVALID_SOCKET; }
		SOCKET get_State() const { return State; }

		//this is a graceful disconnect and will disconnect on the next loop iteration
		//will not call any disconnect callbacks until the disconnect actually occurs
		Network_Return Disconnect(){ State = PEER_STATE_DISCONNECTED; return Network_Return::FAILED; }

		bool Authorized = false;
		Traffic_Monitor Traffic;
		User_Info_Header Connection_Info;

		static Network_Return ProcessReceived(std::shared_ptr<SocketHandler>& socket, Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&>& receive_callback, Delegate<void, std::shared_ptr<SocketHandler>&>& onconnect_callback);
		static Network_Return CheckState(std::shared_ptr<SocketHandler>& socket);
	};
};

#endif