#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "ImageCompression.h"
#include "CommonNetwork.h"
#include "Display.h"
#include "BaseClient.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"

RemoteDesktop::Client::Client(HWND hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int)) : _HWND(hwnd), _OnConnect(onconnect), _OnDisconnect(ondisconnect) {

	_ImageCompression = std::make_unique<ImageCompression>();
	_Display = std::make_unique<Display>(hwnd, oncursorchange);
	//SetWindowText(_HWND, L"Remote Desktop Viewer");
	DEBUG_MSG("Client()");
}

RemoteDesktop::Client::~Client(){
	DEBUG_MSG("~Client() Beg");
	_NetworkClient->Stop();
	DEBUG_MSG("~Client() End");
}
void RemoteDesktop::Client::OnDisconnect(){
	_OnDisconnect();
}
void RemoteDesktop::Client::Connect(std::wstring host, std::wstring port){
	if (_NetworkClient) _NetworkClient.reset();
	_NetworkClient = std::make_unique<BaseClient>(std::bind(&RemoteDesktop::Client::OnConnect, this, std::placeholders::_1),
		std::bind(&RemoteDesktop::Client::OnReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&RemoteDesktop::Client::OnDisconnect, this));
	_NetworkClient->Connect(host, port);
}
void RemoteDesktop::Client::Stop(){
	_NetworkClient->Stop();
}

void RemoteDesktop::Client::OnConnect(std::shared_ptr<SocketHandler>& sh){
	DEBUG_MSG("Connection Successful");

	_OnConnect();
}

void RemoteDesktop::Client::KeyEvent(int VK, bool down) {

	NetworkMsg msg;
	KeyEvent_Header h;
	h.VK = VK;
	h.down = down == true ? 0 : -1;
	DEBUG_MSG("KeyEvent % in state, down %", VK, (int)h.down);
	msg.push_back(h);
	_NetworkClient->Send(NetworkMessages::KEYEVENT, msg);
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

	if (_LastMouseEvent.Action == action && _LastMouseEvent.pos.left == x && _LastMouseEvent.pos.top == y && wheel == 0) DEBUG_MSG("skipping mouse event, duplicate");
	else {
		memcpy(&_LastMouseEvent, &h, sizeof(h));
		msg.push_back(h);
		_NetworkClient->Send(NetworkMessages::MOUSEEVENT, msg);
	}

}
void RemoteDesktop::Client::SendCAD(){
	NetworkMsg msg;
	_NetworkClient->Send(NetworkMessages::CAD, msg);
}

void RemoteDesktop::Client::SendFile(const char* absolute_path, const char* root_path){
	std::string filename = absolute_path;
	if (IsFile(filename)){
		auto fs = filesize(absolute_path);
		if (fs <= 0) return;//file must not exist
		NetworkMsg msg;
		std::vector<char> data;
		data.resize(fs);

		std::ifstream in(absolute_path, std::ifstream::binary);
		in.read(data.data(), fs);//read all the data

		//_NetworkClient->Send(NetworkMessages::CAD, msg);
	}
	else {

	}

}

void RemoteDesktop::Client::Draw(HDC hdc){
	_Display->Draw(hdc);
}

void RemoteDesktop::Client::OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh) {
	auto t = Timer(true);
	auto beg = data;
	if (header->Packet_Type == NetworkMessages::RESOLUTIONCHANGE){

		Image img;
		memcpy(&img.height, beg, sizeof(img.height));
		beg += sizeof(img.height);
		memcpy(&img.width, beg, sizeof(img.width));
		beg += sizeof(img.width);
		img.compressed = true;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = header->PayloadLen - sizeof(img.height) - sizeof(img.width);

		_Display->NewImage(_ImageCompression->Decompress(img));

	}
	else if (header->Packet_Type == NetworkMessages::UPDATEREGION){
		Image img;
		Image_Diff_Header imgdif_network;

		memcpy(&imgdif_network, beg, sizeof(imgdif_network));

		beg += sizeof(imgdif_network);
		img.height = imgdif_network.rect.height;
		img.width = imgdif_network.rect.width;
		img.compressed = imgdif_network.compressed == 0 ? true : false;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = header->PayloadLen - sizeof(imgdif_network);

		_Display->UpdateImage(_ImageCompression->Decompress(img), imgdif_network);

	}
	else if (header->Packet_Type == NetworkMessages::MOUSEEVENT){
		MouseEvent_Header h;
		memcpy(&h, beg, sizeof(h));
		_Display->UpdateMouse(h);
	}


	t.Stop();
	//DEBUG_MSG("took: %", t.Elapsed());
}

