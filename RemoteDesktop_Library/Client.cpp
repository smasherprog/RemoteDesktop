#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "ImageCompression.h"

RemoteDesktop::Client::Client(){

#if defined _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif
	_ImageCompression = std::make_unique<ImageCompression>();

}

RemoteDesktop::Client::~Client(){

}
void RemoteDesktop::Client::OnDisconnect(SocketHandler& sh){
}
void RemoteDesktop::Client::OnConnect(SocketHandler& sh){
	DEBUG_MSG("Connection Successful");
}
void RemoteDesktop::Client::OnReceive(SocketHandler& sh){
	if (sh.msgtype == NetworkMessages::RESOLUTIONCHANGE){

		Image img;
		auto beg = sh.Buffer.data();
		memcpy(&img.height, beg, sizeof(img.height));
		beg += sizeof(img.height);
		memcpy(&img.width, beg, sizeof(img.width));
		beg += sizeof(img.width);
		img.compressed = true;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = sh.msglength - sizeof(img.height) - sizeof(img.width);

		auto uncompresesdimg = _ImageCompression->Decompress(img);


		auto hDC = GetDC(_HWND);

		auto bmp = CreateBitmap(img.width, img.height, 1, 32, uncompresesdimg.data);
		auto hMemDC = CreateCompatibleDC(hDC);

		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, bmp);

		BitBlt(hDC, 0, 0, img.width, img.height, hMemDC, 0, 0, SRCCOPY);

		SelectObject(hMemDC, hOldBitmap);

		DeleteObject(bmp);

		DeleteDC(hMemDC);

		ReleaseDC(_HWND, hDC);


	}
	else if (sh.msgtype == NetworkMessages::UPDATEREGION){
		Image img;
		auto beg = sh.Buffer.data();
		Rect rect;
		memcpy(&rect, beg, sizeof(rect));
		beg += sizeof(rect);
		img.compressed = true;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = sh.msglength - sizeof(rect);
		auto uncompresesdimg = _ImageCompression->Decompress(img);
	}
	DEBUG_MSG("OnReceive Successful Packet size %", sh.msglength);
}