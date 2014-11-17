#include "stdafx.h"
#include "SocketHandler.h"
#include "Encryption.h"
#include <thread>

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
	_Encyption.Init(client);


}

RemoteDesktop::Network_Return _SendLoop(SOCKET s, char* data, int len){
	while (len > 0){
		//DEBUG_MSG("send len %", len);
		auto sentamount = send(s, data, len, 0);
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

	auto ret = _SendLoop(_Socket->socket, _SendBuffer.data(), _SendBuffer.size());
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
	auto sendsize = sizeof(Packet_Encrypt_Header) + sizeof(Packet_Header) + msg.payloadlength() + 32;
	if (sendsize > MAXMESSAGESIZE) return _Disconnect();
	if (sendsize >= _SendBuffer.capacity()) _SendBuffer.reserve(sendsize + STARTBUFFERSIZE);//grow ahead by chunks

	auto beg = _SendBuffer.data() + sizeof(Packet_Encrypt_Header);
	auto ph = (Packet_Header*)beg;
	beg += sizeof(Packet_Header);
	ph->Packet_Type = m;
	ph->PayloadLen = msg.payloadlength();
	for (auto i = 0; i < msg.data.size(); i++){
		memcpy(beg, msg.data[i].data, msg.data[i].len);
		beg += msg.data[i].len;
	}
	auto end = beg;
	beg = _SendBuffer.data() + sizeof(Packet_Encrypt_Header);
	auto enph = (Packet_Encrypt_Header*)_SendBuffer.data();
	enph->PayloadLen = _Encyption.Ecrypt(beg, end - beg, enph->IV) + IVSIZE;
	if (enph->PayloadLen < 0)  return _Disconnect();
	if (_SendLoop(_Socket->socket, _SendBuffer.data(), enph->PayloadLen + sizeof(enph->PayloadLen)) == RemoteDesktop::Network_Return::FAILED) return _Disconnect();
	Traffic.UpdateSend(enph->PayloadLen + sizeof(enph->PayloadLen));
	return RemoteDesktop::Network_Return::COMPLETED;
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::Receive(){

	if (_ReceivedBufferCounter - _ReceivedBuffer.capacity() < STARTBUFFERSIZE) _ReceivedBuffer.reserve(_ReceivedBuffer.capacity() + STARTBUFFERSIZE);//grow ahead by chunks
	auto amtrec = recv(_Socket->socket, _ReceivedBuffer.data() + _ReceivedBufferCounter, _ReceivedBuffer.capacity() - _ReceivedBufferCounter, 0);//read as much as possible
	if (amtrec > 0){
		_ReceivedBufferCounter += amtrec;
		Traffic.UpdateRecv(amtrec);
		return _Decrypt_Received_Data();
	}
	else if (amtrec == 0) return Network_Return::FAILED;
	else {
		auto errmsg = WSAGetLastError();
		//DEBUG_MSG("_ProcessPacketHeader %", errmsg);
		if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE)  return Network_Return::PARTIALLY_COMPLETED;
		//DEBUG_MSG("_ProcessPacketHeader DISCONNECTING");
		return _Disconnect();
	}
}
RemoteDesktop::Network_Return RemoteDesktop::SocketHandler::_Disconnect(){
	State = PEER_STATE_DISCONNECTED;//force disconnect
	if (Disconnect_CallBack) Disconnect_CallBack(this);
	return Network_Return::FAILED;//disconnect!
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
			if (_Encyption.Decrypt(_ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header), p->PayloadLen - IVSIZE, p->IV)){
				auto beg = _ReceivedBuffer.data() + sizeof(Packet_Encrypt_Header);
				auto ph = (Packet_Header*)beg;
				if (ph->PayloadLen > p->PayloadLen - IVSIZE) return _Disconnect();//malformed packet
				if (Receive_CallBack) Receive_CallBack(ph, beg + sizeof(Packet_Header), this);
				_ReceivedBufferCounter -= p->PayloadLen + sizeof(p->PayloadLen);
				if (_ReceivedBufferCounter > 0) memmove(_ReceivedBuffer.data(), _ReceivedBuffer.data() + p->PayloadLen + sizeof(p->PayloadLen), _ReceivedBufferCounter);//this will shift down the data 
				return _Decrypt_Received_Data();//recursive call!
			}
			else return _Disconnect();
		}
	}
	return Network_Return::COMPLETED;
}
