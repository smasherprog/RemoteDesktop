#include "stdafx.h"
#include "SocketHandler.h"
#include "Encryption.h"
#include <thread>
#include "Compression_Handler.h"
#include "NetworkSetup.h"

std::vector<std::vector<char>> RemoteDesktop::INTERNAL::SocketBufferCache;
std::mutex RemoteDesktop::INTERNAL::SocketBufferCacheLock;

RemoteDesktop::SocketHandler::SocketHandler(SOCKET socket, bool client) : _Socket(RAIISOCKET(socket)) {
	if (client)	State = PEER_STATE_DISCONNECTED;
	else State = PEER_STATE_CONNECTED;//servers just listen so they are in a good state
	_SendBuffer.reserve(STARTBUFFERSIZE);
	_ReceivedCompressionBuffer.reserve(STARTBUFFERSIZE);
	_SendCompressionBuffer.reserve(STARTBUFFERSIZE);
	memset(&Connection_Info, 0, sizeof(Connection_Info));
	//init to empty functions

	_Encyption.Init(client);
}
RemoteDesktop::SocketHandler::~SocketHandler(){
	DEBUG_MSG("~SocketHandler");
}
void RemoteDesktop::SocketHandler::Receive(){
	auto ret = 0;
	{
		std::lock_guard<std::mutex> lock(_ReceiveLock);
		ret = ReceiveLoop(_Socket->socket, _In_ReceivedBuffer, _In_ReceivedBufferCounter);
	}
	if (ret == RemoteDesktop::Network_Return::FAILED) Disconnect();
//	DEBUG_MSG("_Receive %", _In_ReceivedBufferCounter);
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
	auto ret = SendLoop(_Socket->socket, (char*)&tmp, sizeof(tmp));//id is always sent at the beginning of a connection. This is to accommodate proxy sever
	if (ret == FAILED) return Disconnect();
	ret = SendLoop(_Socket->socket, _SendBuffer.data(), _SendBuffer.size());

	if (aeskey.size() > 1){
		State = PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES;
		std::vector<char> aes;
		aes.resize(48);
		std::string tmpaes = ws2s(aeskey);
		hex2bin(tmpaes.c_str(), aes.data(), aes.data() + aes.size());

		_Encyption.set_AES_Key(aes.data());
	}
	else State = PEER_STATE_EXCHANGING_KEYS;
	if (ret == FAILED) return Disconnect();
	return ret;
}

RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Send(NetworkMessages m){
	NetworkMsg msg;
	return Send(m, msg);
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Send(NetworkMessages m, const NetworkMsg& msg){
	if (State == PEER_STATE_DISCONNECTED) return Network_Return::FAILED;
	else if (State == PEER_STATE_EXCHANGING_KEYS || State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES) return Network_Return::PARTIALLY_COMPLETED;
	else return _Encrypt_And_Send(m, msg);
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Encrypt_And_Send(NetworkMessages m, const NetworkMsg& msg){
	std::lock_guard<std::mutex> slock(_SendLock);//this lock is needed to prevent multiple threads from interleaving send calls and interleaving data in the buffers

	auto sendsize = sizeof(Packet_Encrypt_Header) + sizeof(Packet_Header) + Compression_Handler::CompressionBound(msg.payloadlength()) + IVSIZE * 2;//max possible size needed
	if (sendsize > MAXMESSAGESIZE) return Disconnect();
	if (sendsize >= _SendBuffer.capacity()){
		_SendBuffer.reserve(sendsize);
		_SendCompressionBuffer.reserve(sendsize);
	}

	auto beg = _SendBuffer.data();
	for (size_t i = 0; i < msg.data.size(); i++){
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

	enph->PayloadLen = _Encyption.Ecrypt(_SendCompressionBuffer.data(), _SendBuffer.data() + sizeof(Packet_Encrypt_Header), packetheader->PayloadLen + sizeof(Packet_Header), _SendBuffer.capacity() - sizeof(Packet_Encrypt_Header), enph->IV) + IVSIZE;
	assert((enph->PayloadLen + sizeof(enph->PayloadLen)) <= _SendBuffer.capacity());
	//DEBUG_MSG("Final Outbound Size: %", enph->PayloadLen);
	if (enph->PayloadLen < 0)  return Disconnect();
	if (SendLoop(_Socket->socket, _SendBuffer.data(), enph->PayloadLen + sizeof(enph->PayloadLen)) == RemoteDesktop::Network_Return::FAILED) return Disconnect();
	Traffic.UpdateSend(roundUp(msg.payloadlength() + TOTALHEADERSIZE, 16), enph->PayloadLen + sizeof(enph->PayloadLen));// an uncompressed message would be encrypted and rounded up to the nearest 16 bytes so adjust accordingly
	return RemoteDesktop::Network_Return::COMPLETED;
}


/////////////////STATIC METHODS BELOW!!!
//
//
//
//
//
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::ProcessReceived(std::shared_ptr<SocketHandler>& socket, Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&>& receive_callback, Delegate<void, std::shared_ptr<SocketHandler>&>& onconnect_callback){
	if (socket->_In_ReceivedBufferCounter > 0){
		{
			std::lock_guard<std::mutex> lock(socket->_ReceiveLock);
			socket->_ReceivedBuffer.resize(socket->_ReceivedBufferCounter + socket->_In_ReceivedBufferCounter);
			memcpy(socket->_ReceivedBuffer.data() + socket->_ReceivedBufferCounter, socket->_In_ReceivedBuffer.data(), socket->_In_ReceivedBufferCounter);
			socket->_In_ReceivedBuffer.resize(0);
			//DEBUG_MSG("Copied % bytes into received", socket->_In_ReceivedBufferCounter);
			socket->_ReceivedBufferCounter += socket->_In_ReceivedBufferCounter;
			socket->_In_ReceivedBufferCounter = 0;
		}
	}
	if (socket->State == PEER_STATE_EXCHANGING_KEYS || socket->State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES) {//any data received should be the key exchange... if not the connection will terminate
		auto EphemeralPublicKeyLength = socket->_Encyption.get_EphemeralPublicKeyLength();
		auto StaticPublicKeyLength = socket->_Encyption.get_StaticPublicKeyLength();
		auto totalsizepending = StaticPublicKeyLength + EphemeralPublicKeyLength + sizeof(Proxy_Header);
		//extra int is here to support proxy servers, just ignore it completely
		if (socket->_ReceivedBufferCounter < totalsizepending) return Network_Return::PARTIALLY_COMPLETED;
		else {//enough data was received for a key exchange..
			if (!socket->_Encyption.Agree(socket->_ReceivedBuffer.data() + sizeof(Proxy_Header), socket->_ReceivedBuffer.data() + sizeof(Proxy_Header) + StaticPublicKeyLength, socket->State == PEER_STATE_EXCHANGING_KEYS_USE_PRE_AES)) return socket->Disconnect();
			socket->_ReceivedBufferCounter -= totalsizepending;
			assert(socket->_ReceivedBufferCounter >= 0);
			if (socket->_ReceivedBufferCounter > 0) memmove(socket->_ReceivedBuffer.data(), socket->_ReceivedBuffer.data() + totalsizepending, socket->_ReceivedBufferCounter);//this will shift down the data 

			socket->State = PEER_STATE_CONNECTED;
			if (onconnect_callback) onconnect_callback(socket);//client is now connected
		}
	}

	if (socket->_ReceivedBufferCounter > 0 && socket->State == PEER_STATE_CONNECTED){
		auto neededsize = NETWORKHEADERSIZE;
		Packet_Encrypt_Header* encrypt_p_header = nullptr;
		if (socket->_ReceivedBufferCounter >= neededsize){
			encrypt_p_header = (Packet_Encrypt_Header*)socket->_ReceivedBuffer.data();//enough data is available to do this!
			if (encrypt_p_header->PayloadLen >= MAXMESSAGESIZE) return socket->Disconnect();//Buffer Overflow.. disconnect!
		}
		else return Network_Return::PARTIALLY_COMPLETED;
		if (socket->_ReceivedBufferCounter >= encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen)){//data can be decrypted..., it is all here!
			if (socket->_Encyption.Decrypt(socket->_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), socket->_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), encrypt_p_header->PayloadLen - IVSIZE, encrypt_p_header->IV)){
				auto beg = socket->_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header);
				auto pac_header = (Packet_Header*)beg;
				beg += sizeof(Packet_Header);

				if (pac_header->PayloadLen > (encrypt_p_header->PayloadLen - IVSIZE - sizeof(Packet_Header))) return socket->Disconnect();//malformed packet

				//DEBUG_MSG("Received size %", encrypt_p_header->PayloadLen);
				if (pac_header->Packet_Type < 0){//compressed packet
					pac_header->Packet_Type *= -1;//remove the negative sign
					auto assumed_uncompressedsize = Compression_Handler::Decompressed_Size(beg);//get the size of the uncompresseddata

					if (assumed_uncompressedsize >= MAXMESSAGESIZE) return socket->Disconnect();//Buffer Overflow.. disconnect!
					socket->_ReceivedCompressionBuffer.reserve(assumed_uncompressedsize);
					auto newsize = Compression_Handler::Decompress(beg, socket->_ReceivedCompressionBuffer.data(), pac_header->PayloadLen, socket->_ReceivedCompressionBuffer.capacity());
					//DEBUG_MSG("Compressed assumed size %,  output  % type %", assumed_uncompressedsize, newsize, pac_header->Packet_Type);
					if (newsize != assumed_uncompressedsize) return socket->Disconnect();//malformed packet data . . . disconnect!

					socket->Traffic.UpdateRecv(assumed_uncompressedsize + TOTALHEADERSIZE, pac_header->PayloadLen + TOTALHEADERSIZE);
					auto beforesize = pac_header->PayloadLen;
					pac_header->PayloadLen = newsize;
					receive_callback(pac_header, socket->_ReceivedCompressionBuffer.data(), socket);
					pac_header->PayloadLen = beforesize;//restore
				}
				else {
					if (pac_header->Packet_Type != RemoteDesktop::NetworkMessages::KEEPALIVE){
						//DEBUG_MSG("uncompressed size % type %", pac_header->PayloadLen, pac_header->Packet_Type);
						socket->Traffic.UpdateRecv(pac_header->PayloadLen + TOTALHEADERSIZE, pac_header->PayloadLen + TOTALHEADERSIZE);//same size for each if no compression occurs
						receive_callback(pac_header, beg, socket);
					}
				}

				socket->_ReceivedBufferCounter -= (encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen));
				assert(socket->_ReceivedBufferCounter >= 0);
				assert(socket->_ReceivedBufferCounter <= socket->_ReceivedBuffer.capacity());
				if (socket->_ReceivedBufferCounter > 0) {
					memmove(socket->_ReceivedBuffer.data(), socket->_ReceivedBuffer.data() + encrypt_p_header->PayloadLen + sizeof(encrypt_p_header->PayloadLen), socket->_ReceivedBufferCounter);//this will shift down the data 
				}
				return RemoteDesktop::SocketHandler::ProcessReceived(socket, receive_callback, onconnect_callback);//recursive call!
			}
			else return socket->Disconnect();
		}
	}
	return Network_Return::COMPLETED;
}


RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::CheckState(std::shared_ptr<SocketHandler>& socket){
	return socket->Send(RemoteDesktop::NetworkMessages::KEEPALIVE);
}