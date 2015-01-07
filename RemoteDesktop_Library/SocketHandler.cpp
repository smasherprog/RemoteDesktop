#include "stdafx.h"
#include "SocketHandler.h"
#include "Encryption.h"
#include <thread>
#include "Compression_Handler.h"

RemoteDesktop::SocketHandler::SocketHandler(SOCKET socket, bool client) : _Socket(RAIISOCKET(socket)) {
	State = PEER_STATE_DISCONNECTED;
	_ReceivedBuffer.reserve(STARTBUFFERSIZE);
	_SendBuffer.reserve(STARTBUFFERSIZE); 
	_ReceivedCompressionBuffer.reserve(STARTBUFFERSIZE); 
	_SendCompressionBuffer.reserve(STARTBUFFERSIZE);
	_Encyption.Init(client);
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_SendLoop(char* data, int len){
	//auto Timer(true);
	//int counter = 0;
	while (len > 0){
		auto sentamount = send(_Socket->socket, data, len, 0);
		if (sentamount < 0){
			auto sockerr = WSAGetLastError();
			if (sockerr != WSAEMSGSIZE && sockerr != WSAEWOULDBLOCK){
				auto amtrec = recv(_Socket->socket, nullptr, 0, 0);//check if the socket is in a disconnected state
				if (amtrec == 0) return Network_Return::FAILED;
				else if( amtrec<0) sockerr = WSAGetLastError();
				DEBUG_MSG("Disconnecting %", sockerr);
				return RemoteDesktop::Network_Return::FAILED;//disconnect client!!44
			}
			DEBUG_MSG("Yeilding % %", len, sockerr);
			std::this_thread::yield();
			continue;//go back and try again
		}
		len -= sentamount;
	}
	return RemoteDesktop::Network_Return::COMPLETED;
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Exchange_Keys(int dst_id, int src_id, std::wstring aeskey){

	NetworkMsg msg;
	auto EphemeralPublicKeyLength = _Encyption.get_EphemeralPublicKeyLength();
	auto StaticPublicKeyLength = _Encyption.get_StaticPublicKeyLength();

	_SendBuffer.resize(EphemeralPublicKeyLength + StaticPublicKeyLength);
	memcpy(_SendBuffer.data(), _Encyption.get_Static_PublicKey(), StaticPublicKeyLength);
	memcpy(_SendBuffer.data() + StaticPublicKeyLength, _Encyption.get_Ephemeral_PublicKey(), EphemeralPublicKeyLength);
	Proxy_Header tmp;
	tmp.Dst_Id = dst_id;
	tmp.Src_Id = src_id;
	DEBUG_MSG("Exchange_Keys % %", dst_id, src_id);
	auto ret = _SendLoop((char*)&tmp, sizeof(tmp));//id is always sent at the beginning of a connection. This is to accommodate proxy sever
	if (ret == FAILED) return _Disconnect();
	ret = _SendLoop(_SendBuffer.data(), _SendBuffer.size());

	if (aeskey.size() > 1){
		State = PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES;
		std::vector<char> aes;
		aes.resize(48);
		std::string tmpaes = ws2s(aeskey);
		hex2bin(tmpaes.c_str(), aes.data(), aes.data() + aes.size());

		_Encyption.set_AES_Key(aes.data());
	}
	else State = PEER_STATE_EXCHANGING_KEYS;
	if (ret == FAILED) return _Disconnect();
	return ret;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::CheckState(){
	if (State == PEER_STATE_DISCONNECTED) return RemoteDesktop::Network_Return::COMPLETED;
	auto amtrec = recv(_Socket->socket, nullptr, 0, 0);//check if the socket is in a disconnected state
	if (amtrec == 0) return Network_Return::FAILED;
	else {
		auto errmsg = WSAGetLastError();
		if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE)  return Network_Return::PARTIALLY_COMPLETED;
		DEBUG_MSG("CheckState DISCONNECTING %", errmsg);
		return _Disconnect();
	}
	return RemoteDesktop::Network_Return::COMPLETED;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Send(NetworkMessages m, const NetworkMsg& msg){
	if (State == PEER_STATE_DISCONNECTED) return Network_Return::FAILED;
	else if (State == PEER_STATE_EXCHANGING_KEYS || State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES) return Network_Return::PARTIALLY_COMPLETED;
	else return _Encrypt_And_Send(m, msg);
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg){
	std::lock_guard<std::mutex> slock(_SendLock);//this lock is needed to prevent multiple threads from interleaving send calls and interleaving data in the buffers

	auto sendsize = sizeof(Packet_Encrypt_Header) + sizeof(Packet_Header) + Compression_Handler::CompressionBound(msg.payloadlength()) + IVSIZE*2;//max possible size needed
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
		assert(compressedsize <= _SendCompressionBuffer.capacity());
	}
	else {	
		
		//DEBUG_MSG("NOT Compressing Data : %", msg.payloadlength());
		packetheader->PayloadLen = msg.payloadlength();//no compression, just set the payloadsize
	}

	auto enph = (Packet_Encrypt_Header*)_SendBuffer.data();

	enph->PayloadLen = _Encyption.Ecrypt(_SendCompressionBuffer.data(), _SendBuffer.data() + sizeof(Packet_Encrypt_Header), packetheader->PayloadLen + sizeof(Packet_Header), _SendBuffer.capacity() - sizeof(Packet_Encrypt_Header),  enph->IV) + IVSIZE;
	assert((enph->PayloadLen + sizeof(enph->PayloadLen)) <= _SendBuffer.capacity());
	//DEBUG_MSG("Final Outbound Size: %", enph->PayloadLen);
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
		if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE)  return Network_Return::PARTIALLY_COMPLETED;
		DEBUG_MSG("_ReceiveLoop DISCONNECTING %", errmsg);
		return _Disconnect();
	}
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Receive(){
	
	auto retcode = _ReceiveLoop();
	if (retcode == Network_Return::PARTIALLY_COMPLETED) {
		assert(_ReceivedBufferCounter <= _ReceivedBuffer.capacity());
		return _Decrypt_Received_Data();
	}
	else return Network_Return::FAILED;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Disconnect(){
	State = PEER_STATE_DISCONNECTED;//force disconnect
	DEBUG_MSG("_Disconnect SocketHandler");
	if (Disconnect_CallBack) Disconnect_CallBack(this);
	return Network_Return::FAILED;//disconnect!RemoteDesktop_Viewer
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Decrypt_Received_Data(){
	if (State == PEER_STATE_EXCHANGING_KEYS || State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES) {//any data received should be the key exchange... if not the connection will terminate
		auto EphemeralPublicKeyLength = _Encyption.get_EphemeralPublicKeyLength();
		auto StaticPublicKeyLength = _Encyption.get_StaticPublicKeyLength();
		auto totalsizepending = StaticPublicKeyLength + EphemeralPublicKeyLength + sizeof(Proxy_Header);
		//extra int is here to support proxy servers, just ignore it completely
		if (_ReceivedBufferCounter < totalsizepending) return Network_Return::PARTIALLY_COMPLETED;
		else {//enough data was received for a key exchange..
			if (!_Encyption.Agree(_ReceivedBuffer.data() + sizeof(Proxy_Header), _ReceivedBuffer.data() + sizeof(Proxy_Header) + StaticPublicKeyLength, State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES)) return _Disconnect();
			_ReceivedBufferCounter -= totalsizepending;
			assert(_ReceivedBufferCounter >= 0);
			if (_ReceivedBufferCounter > 0) memmove(_ReceivedBuffer.data(), _ReceivedBuffer.data() + totalsizepending, _ReceivedBufferCounter);//this will shift down the data 
			
			State = PEER_STATE_CONNECTED;
			if (Connected_CallBack) Connected_CallBack(this);//client is now connected
		}
	}
	
	if (_ReceivedBufferCounter > 0 && State == PEER_STATE_CONNECTED){
		auto neededsize = NETWORKHEADERSIZE;
		Packet_Encrypt_Header* encrypt_p_header = nullptr;
		if (_ReceivedBufferCounter >= neededsize){
			encrypt_p_header = (Packet_Encrypt_Header*)_ReceivedBuffer.data();//enough data is available to do this!
			if (encrypt_p_header->PayloadLen >= MAXMESSAGESIZE) return _Disconnect();//Buffer Overflow.. disconnect!
		}
		else return Network_Return::PARTIALLY_COMPLETED;
		if (_ReceivedBufferCounter >= encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen)){//data can be decrypted..., it is all here!
			if (_Encyption.Decrypt(_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), _ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), encrypt_p_header->PayloadLen - IVSIZE, encrypt_p_header->IV)){
				auto beg = _ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header);
				auto pac_header = (Packet_Header*)beg;
				beg += sizeof(Packet_Header);

				if (pac_header->PayloadLen > (encrypt_p_header->PayloadLen - IVSIZE - sizeof(Packet_Header))) return _Disconnect();//malformed packet

				DEBUG_MSG("Received size %", encrypt_p_header->PayloadLen);
				if (pac_header->Packet_Type < 0){//compressed packet
					pac_header->Packet_Type *= -1;//remove the negative sign
					auto assumed_uncompressedsize = Compression_Handler::Decompressed_Size(beg);//get the size of the uncompresseddata
					
					if (assumed_uncompressedsize >= MAXMESSAGESIZE) return _Disconnect();//Buffer Overflow.. disconnect!
					_ReceivedCompressionBuffer.reserve(assumed_uncompressedsize);
					auto newsize = Compression_Handler::Decompress(beg, _ReceivedCompressionBuffer.data(), pac_header->PayloadLen, _ReceivedCompressionBuffer.capacity());
					DEBUG_MSG("Compressed assumed size %,  output  % type %", assumed_uncompressedsize, newsize, pac_header->Packet_Type);
					if (newsize != assumed_uncompressedsize) return _Disconnect();//malformed packet data . . . disconnect!

					Traffic.UpdateRecv(assumed_uncompressedsize + TOTALHEADERSIZE, pac_header->PayloadLen + TOTALHEADERSIZE);
					auto beforesize = pac_header->PayloadLen;
					pac_header->PayloadLen = newsize;
					if (Receive_CallBack) Receive_CallBack(pac_header, _ReceivedCompressionBuffer.data(), this);
					pac_header->PayloadLen = beforesize;//restore
				}
				else if (Receive_CallBack) {
					DEBUG_MSG("uncompressed size % type %", pac_header->PayloadLen, pac_header->Packet_Type);
					Traffic.UpdateRecv(pac_header->PayloadLen + TOTALHEADERSIZE, pac_header->PayloadLen + TOTALHEADERSIZE);//same size for each if no compression occurs
					Receive_CallBack(pac_header, beg, this);
				}
				
				_ReceivedBufferCounter -= (encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen));
				assert(_ReceivedBufferCounter >= 0); 
				assert(_ReceivedBufferCounter<= _ReceivedBuffer.capacity());
				if (_ReceivedBufferCounter > 0) {
					memmove(_ReceivedBuffer.data(), _ReceivedBuffer.data() + encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen), _ReceivedBufferCounter);//this will shift down the data 
				}
				return _Decrypt_Received_Data();//recursive call!
			}
			else return _Disconnect();
		}
	}
	return Network_Return::COMPLETED;
}
