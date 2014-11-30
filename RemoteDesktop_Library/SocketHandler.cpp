#include "stdafx.h"
#include "SocketHandler.h"
#include "Encryption.h"
#include <thread>
#include "Compression_Handler.h"

RemoteDesktop::socket_wrapper::~socket_wrapper(){
	if (socket != INVALID_SOCKET){
		shutdown(socket, SD_RECEIVE);
		closesocket(socket);
	}
	socket = INVALID_SOCKET;
}
RemoteDesktop::SocketHandler::SocketHandler(SOCKET socket, bool client){
	_Socket = std::make_unique<socket_wrapper>(socket);
	State = PEER_STATE_DISCONNECTED;
	_ReceivedBuffer.reserve(STARTBUFFERSIZE);
	_SendBuffer.reserve(STARTBUFFERSIZE); 
	_ReceivedCompressionBuffer.reserve(STARTBUFFERSIZE); 
	_SendCompressionBuffer.reserve(STARTBUFFERSIZE);
	_Encyption.Init(client);
}
void ShrinkBuffer(std::vector<char>& buff, int bytesinuse){
	if (bytesinuse <= STARTBUFFERSIZE) buff.resize(STARTBUFFERSIZE);
	else buff.resize(bytesinuse);
	buff.shrink_to_fit();//shrink the vector down
	DEBUG_MSG("Resized the Buffer Capacity now %", buff.capacity());
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_SendLoop(char* data, int len){
	while (len > 0){
		//DEBUG_MSG("send len %", len);
		auto sentamount = send(_Socket->socket, data, len, 0);
		if (sentamount < 0){
			auto sockerr = WSAGetLastError();
			if (sockerr != WSAEMSGSIZE && sockerr != WSAEWOULDBLOCK){
				return RemoteDesktop::Network_Return::FAILED;//disconnect client!!
			}
			//DEBUG_MSG("Yeilding time: %    %", len, sockerr);
			std::this_thread::yield();
			continue;//go back and try again
		}
		len -= sentamount;
	}
	return RemoteDesktop::Network_Return::COMPLETED;
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Exchange_Keys(){

	NetworkMsg msg;
	auto EphemeralPublicKeyLength = _Encyption.get_EphemeralPublicKeyLength();
	auto StaticPublicKeyLength = _Encyption.get_StaticPublicKeyLength();

	_SendBuffer.resize(EphemeralPublicKeyLength + StaticPublicKeyLength);
	memcpy(_SendBuffer.data(), _Encyption.get_Static_PublicKey(), StaticPublicKeyLength);
	memcpy(_SendBuffer.data() + StaticPublicKeyLength, _Encyption.get_Ephemeral_PublicKey(), EphemeralPublicKeyLength);

	auto ret = _SendLoop(_SendBuffer.data(), _SendBuffer.size());
	State = PEER_STATE_EXCHANGING_KEYS;
	if (ret == FAILED) return _Disconnect();
	return ret;
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Send(NetworkMessages m, const NetworkMsg& msg){
	if (State == PEER_STATE_DISCONNECTED) return Network_Return::FAILED;
	else if (State == PEER_STATE_EXCHANGING_KEYS) return Network_Return::PARTIALLY_COMPLETED;
	else return _Encrypt_And_Send(m, msg);
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg){
	std::lock_guard<std::mutex> slock(_SendLock);//this lock is needed to prevent multiple threads from interleaving send calls and interleaving data in the buffers
	if (_SendCounter++ >= 500){
		_SendCounter = 0;
		ShrinkBuffer(_SendBuffer, STARTBUFFERSIZE);
		ShrinkBuffer(_SendCompressionBuffer, STARTBUFFERSIZE);
	}

	auto sendsize = sizeof(Packet_Encrypt_Header) + sizeof(Packet_Header) + Compression_Handler::CompressionBound(msg.payloadlength()) + IVSIZE;//max possible size needed
	if (sendsize > MAXMESSAGESIZE) return _Disconnect();
	if (sendsize >= _SendBuffer.capacity()){
		_SendBuffer.reserve(sendsize);
		_SendCompressionBuffer.reserve(sendsize);
	}
	
	auto beg = _SendBuffer.data();
	for (auto i = 0; i < msg.data.size(); i++){
		memcpy(beg, msg.data[i].data, msg.data[i].len);
		beg += msg.data[i].len;
	}
	auto packetheader = (Packet_Header*)_SendCompressionBuffer.data();
	packetheader->Packet_Type = m;//set packet type
	auto compressedsize = Compression_Handler::Compress(_SendBuffer.data(), _SendCompressionBuffer.data() + sizeof(Packet_Header), msg.payloadlength(), _SendCompressionBuffer.capacity());
	if (compressedsize > 0){
		//DEBUG_MSG("Compressing Data from: %, to %", msg.payloadlength(), compressedsize);
		packetheader->Packet_Type *= -1;//flag as compressed
		packetheader->PayloadLen = compressedsize;//set new payload size
	}
	else {
		//DEBUG_MSG("NOT Compressing Data : %", msg.payloadlength());
		packetheader->PayloadLen = msg.payloadlength();//no compression, just set the payloadsize
	}


	auto enph = (Packet_Encrypt_Header*)_SendBuffer.data();

	enph->PayloadLen = _Encyption.Ecrypt(_SendCompressionBuffer.data(), _SendBuffer.data() + sizeof(Packet_Encrypt_Header), packetheader->PayloadLen + sizeof(Packet_Header), enph->IV) + IVSIZE;
//	DEBUG_MSG("Final Outbound Size: %", enph->PayloadLen);
	if (enph->PayloadLen < 0)  return _Disconnect();
	if (_SendLoop(_SendBuffer.data(), enph->PayloadLen + sizeof(enph->PayloadLen)) == RemoteDesktop::Network_Return::FAILED) return _Disconnect();
	Traffic.UpdateSend(roundUp(msg.payloadlength() + TOTALHEADERSIZE, 16), enph->PayloadLen + sizeof(enph->PayloadLen));// an uncompressed message would be encrypted and rounded up to the nearest 16 bytes so adjust accordingly
	return RemoteDesktop::Network_Return::COMPLETED;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_ReceiveLoop(){

	if (_ReceivedBufferCounter - _ReceivedBuffer.size() < STARTBUFFERSIZE) _ReceivedBuffer.resize(_ReceivedBuffer.size() + STARTBUFFERSIZE);//grow ahead by chunks
	auto amtrec = recv(_Socket->socket, _ReceivedBuffer.data() + _ReceivedBufferCounter, _ReceivedBuffer.size() - _ReceivedBufferCounter, 0);//read as much as possible
	if (amtrec > 0){
		_ReceivedBufferCounter += amtrec;
		return _ReceiveLoop();
	}
	else if (amtrec == 0) return Network_Return::FAILED;
	else {
		auto errmsg = WSAGetLastError();
		//DEBUG_MSG("_ProcessPacketHeader %", errmsg);
		if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE)  return Network_Return::PARTIALLY_COMPLETED;
		DEBUG_MSG("_ProcessPacketHeader DISCONNECTING");
		return _Disconnect();
	}
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Receive(){
	if ((_ReceiveCounter++ >= 500) && Traffic.get_TrafficStats().CompressedRecvBPS < 50000){
		_ReceiveCounter = 0;
		ShrinkBuffer(_ReceivedBuffer, _ReceivedBufferCounter);		
		ShrinkBuffer(_ReceivedCompressionBuffer, STARTBUFFERSIZE);
	}
	
	auto retcode = _ReceiveLoop();
	if (retcode== Network_Return::PARTIALLY_COMPLETED) return _Decrypt_Received_Data();
	else return Network_Return::FAILED;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Disconnect(){
	State = PEER_STATE_DISCONNECTED;//force disconnect
	DEBUG_MSG("DebugCalled SocketHandler");
	if (Disconnect_CallBack) Disconnect_CallBack(this);
	return Network_Return::FAILED;//disconnect!RemoteDesktop_Viewer
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Decrypt_Received_Data(){
	if (State == PEER_STATE_EXCHANGING_KEYS) {//any data received should be the key exchange... if not the connection will terminate
		auto EphemeralPublicKeyLength = _Encyption.get_EphemeralPublicKeyLength();
		auto StaticPublicKeyLength = _Encyption.get_StaticPublicKeyLength();

		if (_ReceivedBufferCounter < StaticPublicKeyLength + EphemeralPublicKeyLength) return Network_Return::PARTIALLY_COMPLETED;
		else {//enough data was recieved for a key exchange..
			if (!_Encyption.Agree(_ReceivedBuffer.data(), _ReceivedBuffer.data() + StaticPublicKeyLength)) return _Disconnect();
			if (Connected_CallBack) Connected_CallBack(this);//client is now connected
			_ReceivedBufferCounter -= (StaticPublicKeyLength + EphemeralPublicKeyLength);
			memmove(_ReceivedBuffer.data(), _ReceivedBuffer.data() + StaticPublicKeyLength + EphemeralPublicKeyLength, _ReceivedBufferCounter);//this will shift down the data 
			State = PEER_STATE_CONNECTED;
		}
	}
	if (_ReceivedBufferCounter > 0 && State == PEER_STATE_CONNECTED){
		auto neededsize = NETWORKHEADERSIZE;
		Packet_Encrypt_Header* p = nullptr;
		if (_ReceivedBufferCounter >= neededsize){
			p = (Packet_Encrypt_Header*)_ReceivedBuffer.data();//enough data is available to do this!
			if (p->PayloadLen >= MAXMESSAGESIZE) return _Disconnect();//Buffer Overflow.. disconnect!
		}
		else return Network_Return::PARTIALLY_COMPLETED;
		if (_ReceivedBufferCounter >= p->PayloadLen){//data can be decrypted..., it is all here!
			if (_Encyption.Decrypt(_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), _ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), p->PayloadLen - IVSIZE,  p->IV)){
				auto beg = _ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header);
				auto ph = (Packet_Header*)beg;
				beg += sizeof(Packet_Header);
				if (ph->PayloadLen > p->PayloadLen - IVSIZE) return _Disconnect();//malformed packet

				//DEBUG_MSG("Received size %", p->PayloadLen);
				if (ph->Packet_Type < 0){//compressed packet
					ph->Packet_Type *=-1;//remove the negative sign
					auto assumed_uncompressedsize = Compression_Handler::Decompressed_Size(beg);//get the size of the uncompresseddata
					
					if (assumed_uncompressedsize >= MAXMESSAGESIZE) return _Disconnect();//Buffer Overflow.. disconnect!
					_ReceivedCompressionBuffer.reserve(assumed_uncompressedsize);
					auto newsize = Compression_Handler::Decompress(beg, _ReceivedCompressionBuffer.data(), ph->PayloadLen, _ReceivedCompressionBuffer.capacity());
					//DEBUG_MSG("Compressed assumed size %,  output  %", assumed_uncompressedsize, newsize);
					if (newsize != assumed_uncompressedsize) return _Disconnect();//malformed packet data . . . disconnect!

					Traffic.UpdateRecv(assumed_uncompressedsize + TOTALHEADERSIZE, ph->PayloadLen + TOTALHEADERSIZE);
					auto beforesize = ph->PayloadLen;
					ph->PayloadLen = newsize;
					if (Receive_CallBack) Receive_CallBack(ph, _ReceivedCompressionBuffer.data(), this);
					ph->PayloadLen = beforesize;//restore
				}
				else if (Receive_CallBack) {
				//	DEBUG_MSG("uncompressed size %", ph->PayloadLen);
					Traffic.UpdateRecv(ph->PayloadLen + TOTALHEADERSIZE, ph->PayloadLen + TOTALHEADERSIZE);//same size for each if no compression occurs
					Receive_CallBack(ph, beg, this);
				}
				
				_ReceivedBufferCounter -= p->PayloadLen + sizeof(p->PayloadLen);
				if (_ReceivedBufferCounter > 0) memmove(_ReceivedBuffer.data(), _ReceivedBuffer.data() + p->PayloadLen + sizeof(p->PayloadLen), _ReceivedBufferCounter);//this will shift down the data 
				return _Decrypt_Received_Data();//recursive call!
			}
			else return _Disconnect();
		}
	}
	return Network_Return::COMPLETED;
}
