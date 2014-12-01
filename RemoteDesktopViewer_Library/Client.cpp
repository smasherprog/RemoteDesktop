#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "CommonNetwork.h"
#include "Display.h"
#include "BaseClient.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"
#include "..\RemoteDesktop_Library\Image.h"
#include "..\RemoteDesktop_Library\Delegate.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"

RemoteDesktop::Client::Client(HWND hwnd,
	void(__stdcall * onconnect)(),
	void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int),
	void(__stdcall * onprimchanged)(int, int)) : _HWND(hwnd), _OnConnect(onconnect), _OnDisconnect(ondisconnect), _OnPrimaryChanged(onprimchanged) {
	_Display = std::make_unique<Display>(hwnd, oncursorchange);
	_ClipboardMonitor = std::make_unique<ClipboardMonitor>(DELEGATE(&RemoteDesktop::Client::_OnClipboardChanged, this));
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
	_NetworkClient = std::make_unique<BaseClient>(DELEGATE(&RemoteDesktop::Client::OnConnect, this),
		DELEGATE(&RemoteDesktop::Client::OnReceive, this),
		DELEGATE(&RemoteDesktop::Client::OnDisconnect, this));
	_NetworkClient->Connect(host, port);
}
void RemoteDesktop::Client::Stop(){
	_NetworkClient->Stop();
}
void RemoteDesktop::Client::_OnClipboardChanged(const Clipboard_Data& c){

	NetworkMsg msg;
	int dibsize(c.m_pDataDIB.size()), htmlsize(c.m_pDataHTML.size()), rtfsize(c.m_pDataRTF.size()), textsize(c.m_pDataText.size());

	msg.push_back(dibsize);
	msg.data.push_back(DataPackage(c.m_pDataDIB.data(), dibsize));
	msg.push_back(htmlsize);
	msg.data.push_back(DataPackage(c.m_pDataHTML.data(), htmlsize));
	msg.push_back(rtfsize);
	msg.data.push_back(DataPackage(c.m_pDataRTF.data(), rtfsize));
	msg.push_back(textsize);
	msg.data.push_back(DataPackage(c.m_pDataText.data(), textsize));

	_NetworkClient->Send(NetworkMessages::CLIPBOARDCHANGED, msg);

}
void RemoteDesktop::Client::_Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	Clipboard_Data clip;
	int dibsize(0), htmlsize(0), rtfsize(0), textsize(0);

	dibsize = (*(int*)data);
	data += sizeof(dibsize);
	clip.m_pDataDIB.resize(dibsize);
	memcpy(clip.m_pDataDIB.data(), data, dibsize);
	data += dibsize;

	htmlsize = (*(int*)data);
	data += sizeof(htmlsize);
	clip.m_pDataHTML.resize(htmlsize);
	memcpy(clip.m_pDataHTML.data(), data, htmlsize);
	data += htmlsize;

	rtfsize = (*(int*)data);
	data += sizeof(rtfsize);
	clip.m_pDataRTF.resize(rtfsize);
	memcpy(clip.m_pDataRTF.data(), data, rtfsize);
	data += rtfsize;

	textsize = (*(int*)data);
	data += sizeof(textsize);
	clip.m_pDataText.resize(textsize);
	memcpy(clip.m_pDataText.data(), data, textsize);
	data += textsize;
	_ClipboardMonitor->Restore(clip);
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
RemoteDesktop::Traffic_Stats RemoteDesktop::Client::get_TrafficStats() const{
	auto s = _NetworkClient->Socket;
	if (s) return s->Traffic.get_TrafficStats();
	RemoteDesktop::Traffic_Stats tmp;
	memset(&tmp, 0, sizeof(tmp));
	return tmp;
}
void RemoteDesktop::Client::SendFile(const char* absolute_path, const char* relative_path){
	std::string filename = absolute_path;
	std::string relative = relative_path;
	if (IsFile(filename)){
		auto fs = filesize(absolute_path);
		if (fs <= 0) return;//file must not exist
		NetworkMsg msg;
		std::vector<char> data;
		data.resize(fs);

		std::ifstream in(absolute_path, std::ifstream::binary);
		in.read(data.data(), fs);//read all the data
		char size = relative.size();
		msg.data.push_back(DataPackage(&size, sizeof(size)));
		msg.data.push_back(DataPackage(relative.c_str(), relative.size()));
		int isize = data.size();
		msg.data.push_back(DataPackage((char*)&isize, sizeof(isize)));
		msg.data.push_back(DataPackage(data.data(), data.size()));
		_NetworkClient->Send(NetworkMessages::FILE, msg);
	}
	else {//this is a folder

		NetworkMsg msg;
		char size = relative.size();
		msg.data.push_back(DataPackage(&size, sizeof(size)));
		msg.data.push_back(DataPackage(relative.c_str(), relative.size()));
		_NetworkClient->Send(NetworkMessages::FOLDER, msg);
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
		img.stride = 3;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = header->PayloadLen - sizeof(img.height) - sizeof(img.width);
		_OnPrimaryChanged(img.width, img.height);
		_Display->NewImage(img);

	}
	else if (header->Packet_Type == NetworkMessages::UPDATEREGION){
		Image img;
		Rect rect;

		memcpy(&rect, beg, sizeof(rect));
		img.stride = 3;
		beg += sizeof(rect);
		img.height = rect.height;
		img.width = rect.width;
		img.data = (unsigned char*)beg;
		img.size_in_bytes = header->PayloadLen - sizeof(rect);

		//DEBUG_MSG("_Handle_ScreenUpdates %, %, %", rect.height, rect.width, img.size_in_bytes);
		_Display->UpdateImage(img, rect);

	}
	else if (header->Packet_Type == NetworkMessages::MOUSEEVENT){
		MouseEvent_Header h;
		memcpy(&h, beg, sizeof(h));
		assert(header->PayloadLen == sizeof(h));
		_Display->UpdateMouse(h);
	}
	else if (header->Packet_Type == NetworkMessages::CLIPBOARDCHANGED){
		_Handle_ClipBoard(header, data, sh);
	}


	t.Stop();
	//DEBUG_MSG("took: %", t.Elapsed());
}

