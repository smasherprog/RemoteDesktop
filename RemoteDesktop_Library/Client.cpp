#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "ImageCompression.h"
#include "CommonNetwork.h"

RemoteDesktop::Client::Client(HWND hwnd) : _HWND(hwnd) {

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
	_DownKeys.clear();
}

void RemoteDesktop::Client::KeyEvent(int VK, bool down) {
	if (down){
		if(std::find(_DownKeys.begin(), _DownKeys.end(), VK) != _DownKeys.end()) _DownKeys.push_back(VK);// key is not in a down state
	}
	else {//otherwise, remove the key
		std::remove(_DownKeys.begin(), _DownKeys.end(), VK);
	}
	
	NetworkMsg msg;
	KeyEvent_Header h;
	h.VK = VK;
	h.down = down == true ? 0 : -1;
	DEBUG_MSG("KeyEvent % in state\, down %", VK, (int)h.down);
	msg.push_back(h);
	Send(NetworkMessages::KEYEVENT, msg);
}

void _Draw(HDC hdc, HBITMAP bmp, int width, int height){
	auto hMemDC = CreateCompatibleDC(hdc);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, bmp);
	BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
}

void RemoteDesktop::Client::OnReceive(SocketHandler& sh){
	auto t = Timer(true);

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


		BITMAPINFO   bi;

		bi.bmiHeader.biSize = sizeof(bi);
		bi.bmiHeader.biWidth = img.width;
		bi.bmiHeader.biHeight = -img.height;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biSizeImage = ((img.width * bi.bmiHeader.biBitCount + 31) / 32) * 4 * img.height;

		std::lock_guard<std::mutex> lock(_DrawLock);

		auto hDC = GetDC(_HWND);
		void* raw_data = nullptr;
		_HBITMAP_wrapper = std::make_unique<HBITMAP_wrapper>(CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &raw_data, NULL, NULL));
		_HBITMAP_wrapper->height = img.height;
		_HBITMAP_wrapper->width = img.width;
		_HBITMAP_wrapper->raw_data = (unsigned char*)raw_data;

		memcpy(raw_data, uncompresesdimg.data, uncompresesdimg.size_in_bytes);
		ReleaseDC(_HWND, hDC);
	}
	else if (sh.msgtype == NetworkMessages::UPDATEREGION){
		auto ptr = _HBITMAP_wrapper.get();

		if (ptr != nullptr) {
			Image img;
			auto beg = sh.Buffer.data();
			Image_Diff_Header imgdif_network;
			memcpy(&imgdif_network, beg, sizeof(imgdif_network));

			beg += sizeof(imgdif_network);
			img.height = imgdif_network.rect.height;
			img.width = imgdif_network.rect.width;
			img.compressed = imgdif_network.compressed == 0 ? true : false;
			img.data = (unsigned char*)beg;
			img.size_in_bytes = sh.msglength - sizeof(imgdif_network);
			auto uncompresesdimg = _ImageCompression->Decompress(img);
			{
				std::lock_guard<std::mutex> lock(_DrawLock);

				Image::Copy(uncompresesdimg, imgdif_network.rect.left, imgdif_network.rect.top, ptr->width * 4, ptr->raw_data);
		
				auto hDC = GetDC(_HWND);
		
				_Draw(hDC, ptr->Bitmap, ptr->width, ptr->height);
				ReleaseDC(_HWND, hDC);

			}

		}
	}
	t.Stop();
	//DEBUG_MSG("took: %", t.Elapsed());
}

void RemoteDesktop::Client::Draw(HDC hdc){
	auto ptr = _HBITMAP_wrapper.get();
	if (ptr == nullptr) return;

	auto t = Timer(true);
	std::lock_guard<std::mutex> lock(_DrawLock);
	_Draw(hdc, ptr->Bitmap, ptr->width, ptr->height);
	t.Stop();
	//DEBUG_MSG("Draw took: %", t.Elapsed());
}