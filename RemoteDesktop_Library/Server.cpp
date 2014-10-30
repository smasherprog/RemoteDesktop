#include "stdafx.h"
#include "Server.h"
#include "ScreenCapture.h"
#include "Console.h"
#include "ImageCompression.h"

RemoteDesktop::Server::Server(){


#if defined _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif
	_NewClients = std::vector<SocketHandler>();

}
RemoteDesktop::Server::~Server(){
	Running = false;
	if (_BackGroundCapturingWorker.joinable()) _BackGroundCapturingWorker.join();
}

void RemoteDesktop::Server::OnConnect(SocketHandler& sh){
	std::lock_guard<std::mutex> lock(_NewClientLock);
	_NewClients.push_back(sh);
}
void RemoteDesktop::Server::OnReceive(SocketHandler& sh) {

}
void RemoteDesktop::Server::OnDisconnect(SocketHandler& sh) {

}
void RemoteDesktop::Server::Listen(unsigned short port) {
	RemoteDesktop::BaseServer::Listen(port);//start the lower service first
	_BackGroundCapturingWorker = std::thread(&Server::_Run, this);
}

void RemoteDesktop::Server::Stop(){
	RemoteDesktop::BaseServer::Stop();//stop the lower service first,
	if (_BackGroundCapturingWorker.joinable()) _BackGroundCapturingWorker.join();
}
void RemoteDesktop::Server::_HandleNewClients(NetworkMsg& msg, std::vector<SocketHandler>& newclients, Image& img, const std::unique_ptr<ImageCompression>& imagecompression){
	if (newclients.empty()) return;
	int sz[2];
	sz[0] = img.height;
	sz[1] = img.width;
	msg.data.resize(0);
	msg.lens.resize(0);
	auto imgdif = imagecompression->Compress(img);
	msg.data.push_back((char*)&sz);
	msg.lens.push_back(sizeof(int) * 2);
	msg.data.push_back((char*)imgdif.data);
	msg.lens.push_back(imgdif.size_in_bytes);
	
	for (auto& a : newclients){
		Send(a.socket.get()->socket, NetworkMessages::RESOLUTIONCHANGE, msg);
	}
}
void RemoteDesktop::Server::_Run(){
	//VARIABLES DEFINED HERE TO ACHIVE THREAD LOCAL STORAGE.

	auto screencapture = std::make_unique<ScreenCapture>();
	auto imagecompression = std::make_unique<ImageCompression>();
	std::vector<unsigned char> lastimagebuffer;
	std::vector<unsigned char> sendimagebuffer;
	Image _LastImage = screencapture->GetPrimary(lastimagebuffer);
	auto pingpong = true;
	NetworkMsg msg;
	std::vector<SocketHandler> tmpnewclients;
	while (Running){
		if (SocketArray.size() <= 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));//sleep is there are no clients connected. The first is the server
			continue;
		}
		pingpong = !pingpong;
		Image img;
		auto t = Timer(true);
		if (!pingpong) img = screencapture->GetPrimary();
		else img = screencapture->GetPrimary(lastimagebuffer);

		{
			std::lock_guard<std::mutex> lock(_NewClientLock);
			for (auto& a : _NewClients) tmpnewclients.push_back(a);
			_NewClients.resize(0);
		}
		_HandleNewClients(msg, tmpnewclients, img, imagecompression);//<----new clients handled here
		tmpnewclients.resize(0);

		auto rect = Image::Difference(_LastImage, img);
		auto imgdif = imagecompression->Compress(Image::Copy(img, rect, sendimagebuffer));
		msg.data.resize(0);
		msg.lens.resize(0);

		msg.data.push_back((char*)&rect);
		msg.lens.push_back(sizeof(rect));
		msg.data.push_back((char*)imgdif.data);
		msg.lens.push_back(imgdif.size_in_bytes);
		
		SendToAll(NetworkMessages::UPDATEREGION, msg);
		_LastImage = img;
		t.Stop();
		DEBUG_MSG("Time Taken _Run %, rect % % %", t.Elapsed(), imgdif.size_in_bytes, imgdif.height, imgdif.width);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

}