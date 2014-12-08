#include "stdafx.h"
#include "RD_Server.h"
#include "ScreenCapture.h"
#include "MouseCapture.h"
#include "Desktop_Monitor.h"
#include <fstream>
#include "BaseServer.h"
#include "SocketHandler.h"
#include "Rect.h"
#include "CommonNetwork.h"
#include "Event_Wrapper.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"
#include "..\RemoteDesktop_Library\Delegate.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"

#if _DEBUG
#include "Console.h"
#endif

#define FRAME_CAPTURE_INTERVAL 100 //ms between checking for screen changes


#define SELF_REMOVE_STRING  TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > Nul & Del \"%s\"")

void DeleteMe(){

	TCHAR szModuleName[MAX_PATH];
	TCHAR szCmd[2 * MAX_PATH];
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	GetModuleFileName(NULL, szModuleName, MAX_PATH);

	StringCbPrintf(szCmd, 2 * MAX_PATH, SELF_REMOVE_STRING, szModuleName);

	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

}

RemoteDesktop::RD_Server::RD_Server(){

#if _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif

	DWORD bufsize = 256;
	TCHAR buff[256];
	GetUserName(buff, &bufsize);
	std::wstring uname = buff;
	_RunningAsService = ci_find_substr(uname, std::wstring(L"system")) == 0;

	mousecapturing = std::make_unique<MouseCapture>();
	_DesktopMonitor = std::make_unique<DesktopMonitor>();
	_NetworkServer = std::make_unique<BaseServer>(
		DELEGATE(&RemoteDesktop::RD_Server::OnConnect, this),
		DELEGATE(&RemoteDesktop::RD_Server::OnReceive, this),
		DELEGATE(&RemoteDesktop::RD_Server::OnDisconnect, this));
	_ScreenCapture = std::make_unique<ScreenCapture>();
	_ClipboardMonitor = std::make_unique<ClipboardMonitor>(DELEGATE(&RemoteDesktop::RD_Server::_OnClipboardChanged, this));

	_CADEventHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEvenRDCad");
	_SelfRemoveEventHandle = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEvenRemoveSelf");
}
RemoteDesktop::RD_Server::~RD_Server(){
	if (_CADEventHandle != NULL) CloseHandle(_CADEventHandle);
	if (_RemoveOnExit) {
		if (_SelfRemoveEventHandle != NULL) {
			SetEvent(_SelfRemoveEventHandle);//signal the self removal process 
			CloseHandle(_SelfRemoveEventHandle);
		}
		else DeleteMe();//try a self removal 
	}
}
void RemoteDesktop::RD_Server::_Handle_DisconnectandRemove(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	_RemoveOnExit = true;
	_NetworkServer->GracefulStop();//this will cause the main loop to stop and the program to exit
}
void RemoteDesktop::RD_Server::_OnClipboardChanged(const Clipboard_Data& c){
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

	_NetworkServer->SendToAll(NetworkMessages::CLIPBOARDCHANGED, msg);
}
void RemoteDesktop::RD_Server::_Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	Clipboard_Data clip;
	int dibsize(0), htmlsize(0), rtfsize(0), textsize(0);

	dibsize = (*(int*)data);
	data+=sizeof(dibsize);
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


void RemoteDesktop::RD_Server::OnConnect(std::shared_ptr<SocketHandler>& sh){
	std::lock_guard<std::mutex> lock(_NewClientLock);
	_NewClients.push_back(sh);
	DEBUG_MSG("New Client OnConnect");
}
void _HandleKeyEvent(RemoteDesktop::Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	RemoteDesktop::KeyEvent_Header h;
	memcpy(&h, data, sizeof(h));
	INPUT inp;

	inp.type = INPUT_KEYBOARD;
	inp.ki.wVk = h.VK;
	inp.ki.dwFlags = h.down == 0 ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &inp, sizeof(INPUT));
}

void  RemoteDesktop::RD_Server::_Handle_MouseUpdate(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh){
	MouseEvent_Header h;
	memcpy(&h, data, sizeof(h));
	mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos = h.pos;
	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.mouseData = 0;

	auto scx = (float)GetSystemMetrics(SM_CXSCREEN);
	auto scy = (float)GetSystemMetrics(SM_CYSCREEN);

	auto divl = (float)h.pos.left;
	auto divt = (float)h.pos.top;
	inp.mi.dx = (LONG)((65536.0f / scx)*divl);//x being coord in pixels
	inp.mi.dy = (LONG)((65536.0f / scy)*divt);//y being coord in pixels
	if (h.Action == WM_MOUSEMOVE) inp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	else if (h.Action == WM_LBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	else if (h.Action == WM_LBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	else if (h.Action == WM_RBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	else if (h.Action == WM_RBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	else if (h.Action == WM_MBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
	else if (h.Action == WM_MBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
	else if (h.Action == WM_MOUSEWHEEL) {
		inp.mi.dwFlags = MOUSEEVENTF_WHEEL;
		inp.mi.mouseData = h.wheel;
	}
	else if (h.Action == WM_LBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
		SendInput(1, &inp, sizeof(inp));
	}
	if (h.Action == WM_RBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_RIGHTDOWN;
		SendInput(1, &inp, sizeof(inp));
	}

	SendInput(1, &inp, sizeof(inp));
}
void RemoteDesktop::RD_Server::_Handle_File(RemoteDesktop::Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	char size = *data;
	data++;
	std::string fname(data, size);
	data += size;
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;
	int isize = 0;
	memcpy(&isize, data, sizeof(isize));
	data += sizeof(isize);
	std::ofstream f(path, std::ios::binary);
	f.write(data, isize);

}

void RemoteDesktop::RD_Server::_Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	char size = *data;
	data++;
	std::string fname(data, size);
	data += size;
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;
	CreateDirectoryA(path.c_str(), NULL);
}
void RemoteDesktop::RD_Server::OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh) {
	switch (header->Packet_Type){
	case NetworkMessages::KEYEVENT:
		_HandleKeyEvent(header, data, sh);
		break;
	case NetworkMessages::MOUSEEVENT:
		_Handle_MouseUpdate(header, data, sh);
		break;
	case NetworkMessages::CAD:
		SetEvent(_CADEventHandle);
		break;
	case NetworkMessages::FILE:
		_Handle_File(header, data, sh);
		break;
	case NetworkMessages::FOLDER:
		_Handle_Folder(header, data, sh);
		break;
	case NetworkMessages::CLIPBOARDCHANGED:
		_Handle_ClipBoard(header, data, sh);
		break;
	case NetworkMessages::DISCONNECTANDREMOVE:
		_Handle_DisconnectandRemove(header, data, sh);
		break;
		
	default:
		break;
	}
}
void RemoteDesktop::RD_Server::OnDisconnect(std::shared_ptr<SocketHandler>& sh) {

}

void RemoteDesktop::RD_Server::_HandleNewClients(Image& img){

	if (_NewClients.empty()) return;
	img.Optimize();
	NetworkMsg msg;
	int sz[2];
	sz[0] = img.height;
	sz[1] = img.width;
	DEBUG_MSG("Servicing new Client %, %, %", img.height, img.width, img.size_in_bytes);
	msg.data.push_back(DataPackage((char*)&sz, sizeof(int) * 2));
	msg.data.push_back(DataPackage((char*)img.data, img.size_in_bytes));

	std::lock_guard<std::mutex> lock(_NewClientLock);
	
	for (auto& a : _NewClients){

		a->Send(NetworkMessages::RESOLUTIONCHANGE, msg);
	}
	_NewClients.clear();
}
bool RemoteDesktop::RD_Server::_HandleResolutionUpdates(Image& img, Image& _lastimg){
	bool reschange = (img.height != _lastimg.height || img.width != _lastimg.width) && (img.height > 0 && img.width > 0);
	//if there was a resolution change
	if (reschange){
		img.Optimize();
		NetworkMsg msg;
		int sz[2];
		sz[0] = img.height;
		sz[1] = img.width;

		msg.data.push_back(DataPackage((char*)&sz, sizeof(int) * 2));
		msg.data.push_back(DataPackage((char*)img.data, img.size_in_bytes));
		_NetworkServer->SendToAll(NetworkMessages::RESOLUTIONCHANGE, msg);
		return true;
	}
	return false;
}


void RemoteDesktop::RD_Server::_Handle_ScreenUpdates(Image& img, Rect& rect, std::vector<unsigned char>& buffer){
	if (rect.width > 0 && rect.height > 0){
		
		NetworkMsg msg;
		auto imgdif = Image::Copy(img, rect, buffer);
		imgdif.Optimize();
		msg.push_back(rect);
		msg.data.push_back(DataPackage((char*)imgdif.data, imgdif.size_in_bytes)); 

		DEBUG_MSG("_Handle_ScreenUpdates %, %, %", rect.height, rect.width, imgdif.size_in_bytes);
		_NetworkServer->SendToAll(NetworkMessages::UPDATEREGION, msg);
	}

}
void RemoteDesktop::RD_Server::_Handle_MouseUpdates(const std::unique_ptr<MouseCapture>& mousecapturing){
	static auto begintimer = std::chrono::high_resolution_clock::now();

	mousecapturing->Update();
	if (mousecapturing->Last_ScreenPos != mousecapturing->Current_ScreenPos){//mouse pos is different
		if (mousecapturing->Last_Mouse == mousecapturing->Current_Mouse){//mouse icon is the same... only send on interval
			if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begintimer).count() < 50) return;
		}
		begintimer = std::chrono::high_resolution_clock::now();

		NetworkMsg msg;
		MouseEvent_Header h;
		h.pos = mousecapturing->Current_ScreenPos;
		h.HandleID = mousecapturing->Current_Mouse;
		msg.push_back(h);
		_NetworkServer->SendToAll(NetworkMessages::MOUSEEVENT, msg);
		if (mousecapturing->Last_Mouse != mousecapturing->Current_Mouse) DEBUG_MSG("Sending mouse Iconchange %", mousecapturing->Current_Mouse);
		mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos;
		mousecapturing->Last_Mouse = mousecapturing->Current_Mouse;

	}
}
void RemoteDesktop::RD_Server::Listen(unsigned short port, std::wstring host) {

	_NetworkServer->StartListening(port, host, _DesktopMonitor->m_hDesk);

	std::vector<unsigned char> curimagebuffer;
	std::vector<unsigned char> lastimagebuffer;
	std::vector<unsigned char> sendimagebuffer;

	Image _LastImage = _ScreenCapture->GetPrimary(lastimagebuffer);//<---Seed the image
	auto pingpong = true;
	Event_Wrapper shutdownhandle(OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\SessionEventRDProgram"));

	WaitForSingleObject(shutdownhandle.get_Handle(), 100);//call this to get the first signal from the service
	DWORD dwEvent;
	auto lastwaittime = 0;

	while (_NetworkServer->Is_Running()){
		if (shutdownhandle.get_Handle() == NULL) std::this_thread::sleep_for(std::chrono::milliseconds(lastwaittime));//sleep
		else {
			dwEvent = WaitForSingleObject(shutdownhandle.get_Handle(), lastwaittime);
			if (dwEvent == 0){
				_NetworkServer->GracefulStop();//stop program!
				break;
			}
		}
		auto t1 = Timer(true);

		if (_NetworkServer->Client_Count() <= 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));//sleep if there are no clients connected.
			continue;
		}

		auto d = _DesktopMonitor->Is_InputDesktopSelected();
		if (!d)
		{

			_ScreenCapture->ReleaseHandles();//cannot have lingering handles to the exisiting desktop
			_DesktopMonitor->Switch_to_ActiveDesktop();
			_NetworkServer->SetThreadDesktop(_DesktopMonitor->m_hDesk);
		}

		_Handle_MouseUpdates(mousecapturing);

		pingpong = !pingpong;
		Image img;

		if (!pingpong) img = _ScreenCapture->GetPrimary(curimagebuffer);
		else img = _ScreenCapture->GetPrimary(lastimagebuffer);
		_HandleNewClients(img);
		if (!_HandleResolutionUpdates(img, _LastImage)){
			//only send a diff if no res update occurs
			//get difference with last screenshot
			auto rect = Image::Difference(_LastImage, img);
			//send out any screen updates that have changed
			_Handle_ScreenUpdates(img, rect, sendimagebuffer);
		}

		_LastImage = img;
		t1.Stop();
		auto tim = (int)t1.Elapsed_milli();
		lastwaittime = FRAME_CAPTURE_INTERVAL - tim;
		if (lastwaittime < 0) lastwaittime = 0;
	}
	_NetworkServer->ForceStop();
}