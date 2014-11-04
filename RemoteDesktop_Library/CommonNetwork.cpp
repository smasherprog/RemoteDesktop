#include "stdafx.h"
#include "CommonNetwork.h"
#include "SocketHandler.h"
#include <thread>


int RemoteDesktop::_INTERNAL::_ProcessPacketHeader(RemoteDesktop::SocketHandler& sh){
	assert(sh.Buffer.data() != nullptr);
	if (sh.msgtype == RemoteDesktop::NetworkMessages::INVALID){//new message, read the header
		if (sh.bytecounter < NETWORKHEADERSIZE){//only read more if any is needed
			auto amtrec = recv(sh.socket->socket, sh.Buffer.data() + sh.bytecounter, STARTBUFFERSIZE, 0);//read as much as possible
			if (amtrec > 0){//received data.. yay!
			//	DEBUG_MSG("recv _ProcessPacketHeader = %", amtrec);
				sh.bytecounter += amtrec;
			}
			else if (amtrec == 0){
				//DEBUG_MSG("_ProcessPacketHeader Disconnect");
				return 0;
			}
			else {
				auto errmsg = WSAGetLastError(); 
				//DEBUG_MSG("_ProcessPacketHeader %", errmsg);
				if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE) 	return -1;
				
			//	DEBUG_MSG("_ProcessPacketHeader DISCONNECTING");
				return 0;//disconnect!
			}
		}//check if there is enough data in to complete the header 
		if (sh.bytecounter >= NETWORKHEADERSIZE){//msg length and type received
			Packet_Header packheader;
			memcpy(&packheader, sh.Buffer.data(), NETWORKHEADERSIZE);//copy the header
			sh.msglength = packheader.PayloadLen;
			sh.msgtype = (RemoteDesktop::NetworkMessages)packheader.Packet_Type;
		//	DEBUG_MSG("_ProcessPacketHeader = %, %", sh.msglength, sh.msgtype);
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
	auto amtrec = recv(sh.socket->socket, sh.Buffer.data() + sh.bytecounter, STARTBUFFERSIZE - sh.bytecounter, 0);
	if (amtrec > 0){
		sh.bytecounter += amtrec;	
	//	DEBUG_MSG("recv _ProcessPacketBody = %", amtrec);
	}
	else {
		if (sh.bytecounter >= sh.msglength) return 1;// message complete
		auto errmsg = WSAGetLastError();
		if (errmsg == WSAEWOULDBLOCK || errmsg == WSAEMSGSIZE) return -1;
		return 0;//disconnect!
	}
	if (sh.bytecounter >= sh.msglength) return 1;// message complete
	return -1;//not complete..
}
void RemoteDesktop::_INTERNAL::_RecevieEnd(RemoteDesktop::SocketHandler& sh){
	//DEBUG_MSG("_RecevieEnd");
	if (sh.bytecounter > sh.msglength){// more data in the buffer than was in the message
		memmove(sh.Buffer.data(), sh.Buffer.data() + sh.msglength, sh.bytecounter - sh.msglength);
		//DEBUG_MSG("_RecevieEnd = %, %", sh.msglength, sh.msgtype);
		sh.bytecounter -= sh.msglength;
	}
	else sh.bytecounter = 0;
	sh.msglength = 0;
	sh.msgtype = RemoteDesktop::NetworkMessages::INVALID;
}
int RemoteDesktop::_INTERNAL::_SendLoop(SOCKET s, char* data, int len){
	while (len > 0){
	//	DEBUG_MSG("send len %", len);
		auto sentamount = send(s, data, len, 0);
		if (sentamount < 0){
			auto sockerr = WSAGetLastError();
			if (sockerr != WSAEMSGSIZE && sockerr != WSAEWOULDBLOCK){
			//	DEBUG_MSG("send failed with error = %", sockerr);
				return -1;//disconnect client!!
			}
			DEBUG_MSG("Yeilding time: %    %", len, sockerr);
			std::this_thread::yield();
			continue;//go back and try again
		}
		len -= sentamount;
	}
	return 0;//all good
}

int RemoteDesktop::_INTERNAL::_Send(SOCKET s, RemoteDesktop::NetworkMessages m, RemoteDesktop::NetworkMsg& msg){
	int ret = 0;
	if (s != INVALID_SOCKET){
		Packet_Header h;
		h.PayloadLen = msg.payloadlength();
		h.Packet_Type = (char)m;
		ret |= _SendLoop(s, (char*)&h, sizeof(h)); // send header
		for (auto i = 0; i < msg.data.size(); i++){
			ret |= _SendLoop(s, msg.data[i], msg.lens[i]);//send the payload
		}
	}
	else DEBUG_MSG("send failed with error = %", WSAGetLastError());
	return ret;
}