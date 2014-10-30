#include "stdafx.h"
#include "CommonNetwork.h"
#include "SocketHandler.h"

int RemoteDesktop::_INTERNAL::_ProcessPacketHeader(RemoteDesktop::SocketHandler& sh){
	if (sh.msgtype == RemoteDesktop::NetworkMessages::INVALID){//new message, read the header
		if (sh.bytecounter < NETWORKHEADERSIZE){//only read more if any is needed

			auto amtrec = recv(sh.socket.get()->socket, sh.Buffer.data() + sh.bytecounter, NETWORKHEADERSIZE - sh.bytecounter, 0);
			if (amtrec > 0){//received data.. yay!
				sh.bytecounter += amtrec;
			}
			else if (amtrec == 0){
				DEBUG_MSG("_ProcessPacketHeader Disconnect");
				return 0;
			}
			else {
				if (WSAGetLastError() == WSAEWOULDBLOCK) return -1;
				return 0;//disconnect!
			}
		}//check if there is enough data in to complete the header 
		if (sh.bytecounter >= NETWORKHEADERSIZE){//msg length and type received
			memcpy(&sh.msglength, sh.Buffer.data(), sizeof(int));//copy length over
			unsigned char msgtype = 0;
			memcpy(&msgtype, sh.Buffer.data() + sizeof(int), sizeof(msgtype));//copy msg type over
			sh.msgtype = (RemoteDesktop::NetworkMessages)msgtype;
			sh.bytecounter -= NETWORKHEADERSIZE;
			if (sh.bytecounter > 0){//extra data was received some how........ make sure to move it back in the array
				memmove(sh.Buffer.data(), sh.Buffer.data() + NETWORKHEADERSIZE, sh.bytecounter);//use memove in case of overlapping copy
			}
			if (sh.msgtype == RemoteDesktop::NetworkMessages::PING) {//reset the packet.. this is just a dummy to trigger disconnect events
				sh.msglength = 0;
				sh.msgtype = RemoteDesktop::NetworkMessages::INVALID;
				//there is no payload with a PING packet, so it just needs to be reset
				return _ProcessPacketHeader(sh);//keep proccessing in case there is more data to go
			}
			return 1;// keep processing the packet
		}
		else return -1;// header not done and no data to build it.. stop processing
	}
	return 1;
}
int RemoteDesktop::_INTERNAL::_ProcessPacketBody(RemoteDesktop::SocketHandler& sh){
	auto amtrec = recv(sh.socket.get()->socket, sh.Buffer.data() + sh.bytecounter, sh.msglength - sh.bytecounter, 0);
	if (amtrec > 0){
		sh.bytecounter += amtrec;
		if (sh.bytecounter >= sh.msglength) {
			return 1;// message complete
		}
	}
	else {
		if (WSAGetLastError() == WSAEWOULDBLOCK) return -1;
		return 0;//disconnect.. Bad error
	}
	return -1;// not done..
}
void RemoteDesktop::_INTERNAL::_RecevieEnd(RemoteDesktop::SocketHandler& sh){
	if (sh.bytecounter > sh.msglength){// more data in the buffer than was in the message
		memmove(sh.Buffer.data(), sh.Buffer.data() + sh.msglength, sh.bytecounter - sh.msglength);
		sh.bytecounter -= sh.msglength;
	}
	else sh.bytecounter = 0;
	sh.msglength = 0;
	sh.msgtype = RemoteDesktop::NetworkMessages::INVALID;
}
void RemoteDesktop::_INTERNAL::_SendLoop(SOCKET s, char* data, int len){
	while (len > 0){
		auto sentamount = send(s, data, len, 0);
		if (sentamount == SOCKET_ERROR){
			return DEBUG_MSG("send failed with error = %", WSAGetLastError());
		}
			
		len -= sentamount;
	}
}
void RemoteDesktop::_INTERNAL::_Send(SOCKET s, RemoteDesktop::NetworkMessages m, RemoteDesktop::NetworkMsg& msg){
	if (s != INVALID_SOCKET){
		auto payloadlen = msg.payloadlength();
		_SendLoop(s, (char*)&payloadlen, sizeof(payloadlen));//send the lenth first
		auto header = (char)m;
		_SendLoop(s, &header, sizeof(header)); // send header
		for (auto i = 0; i < msg.data.size(); i++){
			_SendLoop(s, msg.data[i], msg.lens[i]);//send the payload
		}
	}
	else return DEBUG_MSG("send failed with error = %", WSAGetLastError());
}