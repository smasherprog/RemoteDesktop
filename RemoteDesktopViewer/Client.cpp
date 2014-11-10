#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "ImageCompression.h"
#include "CommonNetwork.h"
#include "Display.h"

RemoteDesktop::Client::Client(HWND hwnd) : _HWND(hwnd) {

#if defined _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif
	_ImageCompression = std::make_unique<ImageCompression>();
	_Display = std::make_unique<Display>(hwnd);
}

RemoteDesktop::Client::~Client(){

}
void RemoteDesktop::Client::OnDisconnect(SocketHandler& sh){

}
bool RemoteDesktop::Client::SetCursor(){
	static int _LastCursor = 0;
	if (_Display->HCursor.ID == _LastCursor) return true;//nothing to see here
	_LastCursor = _Display->HCursor.ID;
	RECT rect;
	if (!GetClientRect(_HWND, &rect)) return false;

	ClientToScreen(_HWND, (LPPOINT)&rect.left);
	ClientToScreen(_HWND, (LPPOINT)&rect.right);

	if (rect.bottom == 0 && rect.left == 0 && rect.right == 0 && rect.top == 0) {
		DEBUG_MSG("Exiting cannot see window");
		return false;
	}

	POINT p;
	if (!GetCursorPos(&p)) {
		DEBUG_MSG("Exiting cannot GetCursorPos");
		return false;
	}
	bool inwindow = p.x > rect.left && p.x < rect.right && p.y> rect.top && p.y < rect.bottom;
	if (inwindow && (GetFocus() == _HWND)) {
		DEBUG_MSG("Setting cursor %", _Display->HCursor.ID);
		::SetCursor(_Display->HCursor.HCursor);
		return true;
	}
	return false;
}

void RemoteDesktop::Client::OnConnect(SocketHandler& sh){
	DEBUG_MSG("Connection Successful");
	_DownKeys.clear();
}

void RemoteDesktop::Client::KeyEvent(int VK, bool down) {
	if (down){
		if (std::find(_DownKeys.begin(), _DownKeys.end(), VK) != _DownKeys.end()) _DownKeys.push_back(VK);// key is not in a down state
	}
	else {//otherwise, remove the key
		std::remove(_DownKeys.begin(), _DownKeys.end(), VK);
	}

	NetworkMsg msg;
	KeyEvent_Header h;
	h.VK = VK;
	h.down = down == true ? 0 : -1;
	DEBUG_MSG("KeyEvent % in state, down %", VK, (int)h.down);
	msg.push_back(h);
	Send(NetworkMessages::KEYEVENT, msg);
}
void RemoteDesktop::Client::MouseEvent(unsigned int action, int x, int y, int wheel){
	NetworkMsg msg;
	MouseEvent_Header h;
	static MouseEvent_Header _LastMouseEvent;
	h.HandleID = 0;
	h.Action = action;
	h.pos.left = x;
	h.pos.top = y;
	h.wheel = wheel;
	if (_LastMouseEvent.Action == action && _LastMouseEvent.pos.left == x && _LastMouseEvent.pos.top == y &&_LastMouseEvent.wheel == wheel) DEBUG_MSG("skipping mouse event, duplicate");
	else {
		memcpy(&_LastMouseEvent, &h, sizeof(h));
		msg.push_back(h);
		//DEBUG_MSG("sending %   %   %", x, y, action);
		Send(NetworkMessages::MOUSEEVENT, msg);
	}

}

void RemoteDesktop::Client::Draw(HDC hdc){
	_Display->Draw(hdc);
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

		_Display->NewImage(_ImageCompression->Decompress(img));

	}
	else if (sh.msgtype == NetworkMessages::UPDATEREGION){

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
	
		_Display->UpdateImage(_ImageCompression->Decompress(img), imgdif_network);

	}
	else if (sh.msgtype == NetworkMessages::MOUSEEVENT){
		MouseEvent_Header h;
		memcpy(&h, sh.Buffer.data(), sizeof(h));
		_Display->UpdateMouse(h);
	}

	t.Stop();
	//DEBUG_MSG("took: %", t.Elapsed());
}

