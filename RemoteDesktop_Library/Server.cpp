#include "stdafx.h"
#include "Server.h"
#include "ScreenCapture.h"
#include "Console.h"
#include "ImageCompression.h"
#include "MouseCapture.h"

#define FRAME_CAPTURE_INTERVAL 100

RemoteDesktop::Server::Server(){


#if defined _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif
	_NewClients = std::vector<SocketHandler>();
	imagecompression = std::make_unique<ImageCompression>();
	mousecapturing = std::make_unique<MouseCapture>();
}
RemoteDesktop::Server::~Server(){
	Running = false;
	if (_BackGroundWorker.joinable()) _BackGroundWorker.join();
}

void RemoteDesktop::Server::OnConnect(SocketHandler& sh){
	std::lock_guard<std::mutex> lock(_NewClientLock);
	_NewClients.push_back(sh);
}
void _HandleKeyEvent(RemoteDesktop::SocketHandler& sh){
	RemoteDesktop::KeyEvent_Header h;
	memcpy(&h, sh.Buffer.data(), sizeof(h));
	INPUT inp;

	inp.type = INPUT_KEYBOARD;
	inp.ki.wVk = h.VK;
	inp.ki.dwFlags = h.down == 0 ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &inp, sizeof(INPUT));
	//keybd_event(h.VK, scan, h.down ? KEYEVENTF_EXTENDEDKEY : KEYEVENTF_KEYUP, 0);
}

void  RemoteDesktop::Server::_Handle_MouseUpdate(SocketHandler& sh){
	MouseEvent_Header h;
	memcpy(&h, sh.Buffer.data(), sizeof(h));
	mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos = h.pos;
	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.mouseData = 0;
	auto scx = (float)GetSystemMetrics(SM_CXSCREEN);
	auto scy = (float)GetSystemMetrics(SM_CYSCREEN);

	auto divl = (float)h.pos.left;
	auto divt = (float)h.pos.top;
	inp.mi.dx = (65536.0f / scx)*divl;//x being coord in pixels
	inp.mi.dy = (65536.0f / scy)*divt;//y being coord in pixels
	inp.mi.dwFlags = h.Action == WM_MOUSEMOVE ? MOUSEEVENTF_ABSOLUTE : h.Action;

	SendInput(1, &inp, sizeof(inp));

}
void RemoteDesktop::Server::OnReceive(SocketHandler& sh) {
	switch (sh.msgtype){
	case NetworkMessages::KEYEVENT:
		_HandleKeyEvent(sh);
		break;
	case NetworkMessages::MOUSEEVENT:
		_Handle_MouseUpdate(sh);
		break;
	default:
		break;
	}
}
void RemoteDesktop::Server::OnDisconnect(SocketHandler& sh) {

}
void RemoteDesktop::Server::Listen(unsigned short port) {
	RemoteDesktop::BaseServer::Listen(port);//start the lower service first
	_BackGroundWorker = std::thread(&Server::_Run, this);
}

void RemoteDesktop::Server::Stop(){
	RemoteDesktop::BaseServer::Stop();//stop the lower service first,
	if (_BackGroundWorker.joinable()) _BackGroundWorker.join();
}
void RemoteDesktop::Server::_HandleNewClients(std::vector<SocketHandler>& newclients, Image& img){
	if (newclients.empty()) return;
	NetworkMsg msg;
	int sz[2];
	sz[0] = img.height;
	sz[1] = img.width;

	auto imgdif = imagecompression->Compress(img);
	msg.data.push_back((char*)&sz);
	msg.lens.push_back(sizeof(int) * 2);
	msg.data.push_back((char*)imgdif.data);
	msg.lens.push_back(imgdif.size_in_bytes);
	DEBUG_MSG("Servicing new Client!");
	for (auto& a : newclients){
		Send(a.socket->socket, NetworkMessages::RESOLUTIONCHANGE, msg);
	}
}
void RemoteDesktop::Server::_HandleNewClients_and_ResolutionUpdates(Image& img, Image& _lastimg){
	std::vector<SocketHandler> tmpnewclients;
	{
		std::lock_guard<std::mutex> lock(_NewClientLock);
		for (auto& a : _NewClients) tmpnewclients.push_back(a);
		_NewClients.resize(0);
	}
	//if there was a resolution change add those clients to receive the update as well
	if (img.height != _lastimg.height || img.width != _lastimg.width){
		for (auto i = 1; i < SocketArray.size(); i++){
			auto found = false;
			for (auto& s : tmpnewclients){
				if (SocketArray[i].socket->socket == s.socket->socket) {
					found = true;
					break;
				}
			}
			if (!found) tmpnewclients.push_back(SocketArray[i]);
		}
	}
	_HandleNewClients(tmpnewclients, img);//<----new clients handled here, also resolution changes here as well since its the same code
}


void RemoteDesktop::Server::_Handle_ScreenUpdates(Image& img, Rect& rect, std::vector<unsigned char>& buffer){
	if (rect.width > 0 && rect.height > 0){
		NetworkMsg msg;
		auto imgdif = imagecompression->Compress(Image::Copy(img, rect, buffer));
		Image_Diff_Header imgdif_network;
		imgdif_network.rect = rect;
		imgdif_network.compressed = imgdif.compressed == true ? 0 : -1;
		msg.push_back(imgdif_network);

		msg.data.push_back((char*)imgdif.data);
		msg.lens.push_back(imgdif.size_in_bytes);

		SendToAll(NetworkMessages::UPDATEREGION, msg);
	}

}
void RemoteDesktop::Server::_Handle_MouseUpdates(const std::unique_ptr<MouseCapture>& mousecapturing){
	mousecapturing->Update();

	if ((mousecapturing->Last_ScreenPos != mousecapturing->Current_ScreenPos) ||
		mousecapturing->Last_Mouse != mousecapturing->Current_Mouse){
		NetworkMsg msg;
		MouseEvent_Header h;
		h.pos = mousecapturing->Current_ScreenPos;
		h.HandleID = mousecapturing->Current_Mouse;
		msg.push_back(h);
		SendToAll(NetworkMessages::MOUSEEVENT, msg);
		mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos;
		mousecapturing->Last_Mouse = mousecapturing->Current_Mouse;
	}
}

void RemoteDesktop::Server::_Run(){
	//VARIABLES DEFINED HERE TO ACHIVE THREAD LOCAL STORAGE.

	auto screencapture = std::make_unique<ScreenCapture>();

	std::vector<unsigned char> lastimagebuffer;
	std::vector<unsigned char> sendimagebuffer;
	Image _LastImage = screencapture->GetPrimary(lastimagebuffer);//<---Seed the image
	auto pingpong = true;

	while (Running){

		if (SocketArray.size() <= 1) { //The first is the server so check for <=1
			std::this_thread::sleep_for(std::chrono::milliseconds(10));//sleep if there are no clients connected.
			continue;
		}
		_Handle_MouseUpdates(mousecapturing);
		pingpong = !pingpong;
		Image img;
		auto t = Timer(true);
		if (!pingpong) img = screencapture->GetPrimary();
		else img = screencapture->GetPrimary(lastimagebuffer);
		_HandleNewClients_and_ResolutionUpdates(img, _LastImage);
		//get difference with last screenshot
		auto rect = Image::Difference(_LastImage, img);
		//send out any screen updates that have changed
		_Handle_ScreenUpdates(img, rect, sendimagebuffer);

		_LastImage = img;
		t.Stop();
		auto elapsed = t.Elapsed();
		//DEBUG_MSG("Time Taken _Run %", elapsed);
		elapsed = FRAME_CAPTURE_INTERVAL - elapsed;
		if (elapsed > 0) std::this_thread::sleep_for(std::chrono::milliseconds(elapsed));//sleep if there is time to sleep
	}

}