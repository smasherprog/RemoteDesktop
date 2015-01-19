#include "stdafx.h"
#include "Client.h"
#include "Console.h"
#include "CommonNetwork.h"
#include "Display.h"
#include "..\RemoteDesktop_Library\Network_Client.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"
#include "..\RemoteDesktop_Library\Image.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"
#include <Lmcons.h>
#include "..\RemoteDesktop_Library\UserInfo.h"


RemoteDesktop::Client::Client(HWND hwnd,
	void(__stdcall * onconnect)(),
	void(__stdcall * ondisconnect)(),
	void(__stdcall * oncursorchange)(int),
	void(__stdcall * ondisplaychanged)(int, int, int, int, int),
	void(__stdcall * onconnectingattempt)(int, int)) : _HWND(hwnd), _OnConnect(onconnect), _OnDisconnect(ondisconnect), _OnDisplaysChanged(ondisplaychanged), _OnConnectingAttempt(onconnectingattempt) {
	_Display = std::make_shared<Display>(hwnd, oncursorchange);
	_ClipboardMonitor = std::make_shared<ClipboardMonitor>(DELEGATE(&RemoteDesktop::Client::_OnClipboardChanged));
	DEBUG_MSG("Client()");
}
void Send(std::weak_ptr<RemoteDesktop::SocketHandler>& ptr, RemoteDesktop::NetworkMessages m, RemoteDesktop::NetworkMsg& msg){
	std::shared_ptr<RemoteDesktop::SocketHandler> copy(ptr.lock());
	if (copy) copy->Send(m, msg);
}

RemoteDesktop::Client::~Client(){
	DEBUG_MSG("~Client()");
	_NetworkClient = nullptr;
	_ClipboardMonitor = nullptr;
	_Display = nullptr;
}
void RemoteDesktop::Client::OnDisconnect(){
	_OnDisconnect();
}
void RemoteDesktop::Client::Connect(std::wstring port, std::wstring host, int id, std::wstring aeskey){
	_NetworkClient = std::make_shared<Network_Client>();
	_NetworkClient->OnConnected = std::bind(&RemoteDesktop::Client::OnConnect, this, std::placeholders::_1);
	_NetworkClient->OnReceived = std::bind(&RemoteDesktop::Client::OnReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_NetworkClient->OnDisconnect = std::bind(&RemoteDesktop::Client::OnDisconnect, this);
	_NetworkClient->OnConnectingAttempt = _OnConnectingAttempt;
	auto ptr = (Network_Client*)_NetworkClient.get();
	ptr->Start(port, host, id, aeskey);
}
void RemoteDesktop::Client::Stop(){
	_NetworkClient->Stop(true);
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
	Send(Socket, NetworkMessages::CLIPBOARDCHANGED, msg);

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
	Socket = sh;
	DEBUG_MSG("Connection Successful");
	NetworkMsg msg;
	msg.push_back(GetUserInfo());
	Send(Socket, NetworkMessages::CONNECT_REQUEST, msg);
	_OnConnect();
}

void RemoteDesktop::Client::KeyEvent(int VK, bool down) {

	NetworkMsg msg;
	KeyEvent_Header h;
	h.VK = VK;
	h.down = down == true ? 0 : -1;
	msg.push_back(h);
	Send(Socket, NetworkMessages::KEYEVENT, msg);
}
void RemoteDesktop::Client::MouseEvent(unsigned int action, int x, int y, int wheel){
	NetworkMsg msg;
	MouseEvent_Header h;
	h.HandleID = 0;
	h.Action = action;
	h.pos.left = x;
	h.pos.top = y;
	h.wheel = wheel;

	if (_LastMouseEvent.Action == action && _LastMouseEvent.pos.left == x && _LastMouseEvent.pos.top == y && wheel == 0) DEBUG_MSG("skipping mouse event, duplicate");
	else {
		memcpy(&_LastMouseEvent, &h, sizeof(h));
		msg.push_back(h);
		Send(Socket, NetworkMessages::MOUSEEVENT, msg);
	}

}
void RemoteDesktop::Client::SendCAD(){
	NetworkMsg msg;
	Send(Socket, NetworkMessages::CAD, msg);
}
void RemoteDesktop::Client::SendRemoveService(){
	NetworkMsg msg;
	Send(Socket, NetworkMessages::DISCONNECTANDREMOVE, msg);
	_NetworkClient->Set_RetryAttempts(1);//this will cause a quick disconnect
}
void RemoteDesktop::Client::ElevateProcess(wchar_t* username, wchar_t* password){
	NetworkMsg msg;
	std::wstring user(username);
	int len = user.size();
	msg.data.push_back(DataPackage((char*)&len, len));
	msg.data.push_back(DataPackage((char*)username, len));
	std::wstring pass(password);
	int otherlen = pass.size();
	msg.data.push_back(DataPackage((char*)&otherlen, otherlen));
	msg.data.push_back(DataPackage((char*)password, otherlen));
	Send(Socket, NetworkMessages::ELEVATEPROCESS, msg);
}

RemoteDesktop::Traffic_Stats RemoteDesktop::Client::get_TrafficStats() const{

	std::shared_ptr<RemoteDesktop::SocketHandler> s(Socket.lock());
	if (s) return s->Traffic.get_TrafficStats();

	RemoteDesktop::Traffic_Stats tmp;
	memset(&tmp, 0, sizeof(tmp));
	return tmp;
}
void RemoteDesktop::Client::SendSettings(Settings_Header h){
	NetworkMsg msg;
	msg.push_back(h);
	Send(Socket, NetworkMessages::SETTINGS, msg);
}

void RemoteDesktop::Client::SendFile(const char* absolute_path, const char* relative_path, void(__stdcall * onfilechanged)(int)){
	std::string filename = absolute_path;
	std::string relative = relative_path;
	std::shared_ptr<RemoteDesktop::SocketHandler> clienthold(Socket.lock());
	if (!clienthold) return;
	if (IsFile(filename)){

		auto total_size = filesize(absolute_path);
		if (total_size <= 0) return;// ALL DONE!
		File_Header fh;
		fh.ID = 0;
		strcpy_s(fh.RelativePath, relative_path);
		std::vector<char> buffer;
		buffer.reserve(FILECHUNKSIZE);

		size_t total_chunks = total_size / FILECHUNKSIZE;
		size_t last_chunk_size = total_size % FILECHUNKSIZE;
		if (last_chunk_size != 0) /* if the above division was uneven */
		{
			++total_chunks; /* add an unfilled final chunk */
		}
		else /* if division was even, last chunk is full */
		{
			last_chunk_size = FILECHUNKSIZE;
		}

		std::ifstream infile(absolute_path, std::ifstream::binary);

		for (size_t chunk = 0; chunk < total_chunks && clienthold->get_State() != RemoteDesktop::PEER_STATE_DISCONNECTED; ++chunk)
		{
			size_t this_chunk_size =
				chunk == total_chunks - 1 /* if last chunk */
				? last_chunk_size /* then fill chunk with remaining bytes */
				: FILECHUNKSIZE; /* else fill entire chunk */
			infile.read(buffer.data(), this_chunk_size); /* this many bytes is to be read */
			fh.ChunkSize = this_chunk_size;

			NetworkMsg msg;
			msg.push_back(fh);
			msg.data.push_back(DataPackage(buffer.data(), fh.ChunkSize));

			clienthold->Send(NetworkMessages::FILE, msg);
			fh.ID += 1;
			onfilechanged(fh.ChunkSize);
			//std::this_thread::sleep_for(std::chrono::milliseconds(5));//sleep to allow other traffic to flow
		}
	}
	else {//this is a folder

		NetworkMsg msg;
		unsigned char size = relative.size();
		msg.data.push_back(DataPackage((char*)&size, sizeof(size)));
		msg.data.push_back(DataPackage(relative.c_str(), relative.size()));
		clienthold->Send(NetworkMessages::FOLDER, msg);
		onfilechanged(0);
	}

}

void RemoteDesktop::Client::Draw(HDC hdc){
	_Display->Draw(hdc);
}

void RemoteDesktop::Client::OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh) {
	auto t = Timer(true);
	if (header->Packet_Type == NetworkMessages::RESOLUTIONCHANGE){
		New_Image_Header h;
		memcpy(&h, data, sizeof(h));
		data += sizeof(h);

		Image img(Image::Create_from_Compressed_Data((char*)data, header->PayloadLen - sizeof(h), h.Height, h.Width));

		_OnDisplaysChanged(h.Index, h.XOffset, h.YOffset, h.Width, h.Height);
		auto copy = _Display;
		if (copy) copy->Add(img, h);

	}
	else if (header->Packet_Type == NetworkMessages::UPDATEREGION){
		Update_Image_Header h;
		memcpy(&h, data, sizeof(h));
		data += sizeof(h);

		Image img(Image::Create_from_Compressed_Data((char*)data, header->PayloadLen - sizeof(h), h.rect.height, h.rect.width));
		//DEBUG_MSG("_Handle_ScreenUpdates %, %, %", rect.height, rect.width, img.size_in_bytes);
		auto copy = _Display;
		if (copy) copy->Update(img, h);

	}
	else if (header->Packet_Type == NetworkMessages::MOUSEEVENT){
		MouseEvent_Header h;
		memcpy(&h, data, sizeof(h));
		assert(header->PayloadLen == sizeof(h));
		auto copy = _Display;
		if (copy) copy->UpdateMouse(h);
	}
	else if (header->Packet_Type == NetworkMessages::CLIPBOARDCHANGED){
		_Handle_ClipBoard(header, data, sh);
	}


	t.Stop();
	//DEBUG_MSG("took: %", t.Elapsed());
}

