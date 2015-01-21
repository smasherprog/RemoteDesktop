#include "stdafx.h"
#include "Server.h"
#include "MouseCapture.h"
#include "Desktop_Monitor.h"
#include "..\RemoteDesktop_Library\SocketHandler.h"
#include "Rect.h"
#include "..\RemoteDesktop_Library\CommonNetwork.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"
#include "..\RemoteDesktop_Library\Delegate.h"
#include "..\RemoteDesktopServer_Library\SystemTray.h"
#include "..\RemoteDesktop_Library\VirtualScreen.h"
#include "..\RemoteDesktop_Library\EventLog.h"

#include "..\RemoteDesktop_Library\NetworkSetup.h"
#include "..\RemoteDesktop_Library\Utilities.h"
#include "Lmcons.h"
#include "GatewayConnectDialog.h"
#include "NewConnectDialog.h"
#include "..\RemoteDesktop_Library\Network_Server.h"
#include "..\RemoteDesktop_Library\Network_Client.h"
#include "Strsafe.h"
#include "..\RemoteDesktop_Library\Config.h"


#define FRAME_CAPTURE_INTERVAL 100 //ms between checking for screen changes
RemoteDesktop::Server::Server() :
_CADEventHandle(RAIIHANDLE(OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEventRDCad"))),
_SelfRemoveEventHandle(RAIIHANDLE(OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEventRemoveSelf")))
{

	RemoteDesktop::AddFirewallException();

	DWORD bufsize = (UNLEN + 1);
	char buff[UNLEN + 1];
	GetUserNameA(buff, &bufsize);
	std::string uname = buff;
	_RunningAsService = find_substr(uname, std::string("system")) != -1;

	mousecapturing = std::make_unique<MouseCapture>();
	_DesktopMonitor = std::make_unique<DesktopMonitor>();
	_NewClients.reserve(10);//I reserve extra space to allow for accessing past the actual array bounds in some cases. This will only cause an error if the memory is outside of the array capacity
	_ClipboardMonitor = std::make_unique<ClipboardMonitor>(DELEGATE(&RemoteDesktop::Server::_OnClipboardChanged));
	_SystemTray = std::make_unique<SystemTray>();
	_SystemTray->Start(DELEGATE(&RemoteDesktop::Server::_CreateSystemMenu));
	_VirtualScreen = std::make_unique<VirtualScreen>();
}
RemoteDesktop::Server::~Server(){
	if (_RemoveOnExit) {
		if (_SelfRemoveEventHandle.get() != nullptr) {
			SetEvent(_SelfRemoveEventHandle.get());//signal the self removal process the service will call the cleanup
		}
		else Cleanup_System_Configuration();
	}
	//i need to make sure these shut down before cointinuing
	_NetworkServer = nullptr;
	mousecapturing = nullptr;
	_DesktopMonitor = nullptr;
	_ClipboardMonitor = nullptr;
	_SystemTray = nullptr;
	_VirtualScreen = nullptr;

}
void RemoteDesktop::Server::_CreateSystemMenu(){
	_SystemTray->AddMenuItem(L"Exit", DELEGATE(&RemoteDesktop::Server::_TriggerShutDown));
	_SystemTray->AddMenuItem(L"Exit and Remove", DELEGATE(&RemoteDesktop::Server::_TriggerShutDown_and_RemoveSelf));

}

void RemoteDesktop::Server::_TriggerShutDown(){
	_NetworkServer->Stop(false);//this will cause the main loop to stop and the program to exit
}
void RemoteDesktop::Server::_TriggerShutDown_and_RemoveSelf(){
	_TriggerShutDown();
	_RemoveOnExit = true;
}

void RemoteDesktop::Server::_Handle_DisconnectandRemove(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	_TriggerShutDown_and_RemoveSelf();
}

void RemoteDesktop::Server::_Handle_Settings(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	Settings_Header h;
	assert(header->PayloadLen >= sizeof(h));
	memcpy(&h, data, sizeof(h));
	Image_Settings::GrazyScale = h.GrayScale;
	Image_Settings::Quality = h.Image_Quality;
	_ClipboardMonitor->set_ShareClipBoard(h.ShareClip);

	//DEBUG_MSG("Setting Quality to % and GrayScale to %", q, g);
}
void RemoteDesktop::Server::_OnClipboardChanged(const Clipboard_Data& c){
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

	_NetworkServer->Send(NetworkMessages::CLIPBOARDCHANGED, msg, INetwork::Auth_Types::AUTHORIZED);
}
void RemoteDesktop::Server::_Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
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

void RemoteDesktop::Server::_OnAllowConnection(std::wstring name){
	std::lock_guard<std::mutex> lock(_ClientLock);
	DEBUG_MSG("Allowing %", ws2s(name));
	_PendingNewClients.erase(std::remove_if(_PendingNewClients.begin(), _PendingNewClients.end(), [&](std::shared_ptr<SocketHandler>& ptr){
		if (ptr->Connection_Info.full_name == name){
			ptr->Authorized = true;
			SetLast_UserConnectName(name);
			_NewClients.push_back(ptr);
			return true;
		}
		else return false;
	}), _PendingNewClients.end());
}
void RemoteDesktop::Server::_OnDenyConnection(std::wstring name){
	{
		std::lock_guard<std::mutex> lock(_ClientLock);
		auto begit = std::remove_if(_PendingNewClients.begin(), _PendingNewClients.end(), [&](std::shared_ptr<RemoteDesktop::SocketHandler>& ptr) {
			return ptr->Connection_Info.full_name == name;
		});
		auto copybeg = begit;
		while (copybeg != _PendingNewClients.end()){
			(*copybeg)->Authorized = false;
			(*copybeg)->Send(CONNECT_REQUEST_FAILED);
			(*copybeg)->Disconnect();
			++copybeg;
		}
		_PendingNewClients.erase(begit, _PendingNewClients.end());
	}

	auto a = _GatewayConnect_Dialog;
	if (a) a->Show();
}


void _HandleKeyEvent(RemoteDesktop::Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	RemoteDesktop::KeyEvent_Header h;
	assert(header->PayloadLen >= sizeof(h));
	memcpy(&h, data, sizeof(h));
	INPUT inp;

	inp.type = INPUT_KEYBOARD;
	inp.ki.wVk = h.VK;
	inp.ki.dwFlags = h.down == 0 ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &inp, sizeof(INPUT));
}

void  RemoteDesktop::Server::_Handle_MouseUpdate(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	MouseEvent_Header h;
	assert(header->PayloadLen == sizeof(h));
	memcpy(&h, data, sizeof(h));
	mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos = h.pos;
	INPUT inp;
	memset(&inp, 0, sizeof(inp));
	inp.type = INPUT_MOUSE;
	inp.mi.mouseData = 0;
	inp.mi.dx = h.pos.left;
	inp.mi.dy = h.pos.top;
	_VirtualScreen->Map_to_ScreenSpace(inp.mi.dx, inp.mi.dy);

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
	if (h.Action == WM_LBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
		//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, 0, 0);
		SendInput(1, &inp, sizeof(inp));
	}
	else if (h.Action == WM_RBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_RIGHTDOWN;
		//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, 0, 0);
		SendInput(1, &inp, sizeof(inp));
	}
	//DEBUG_MSG("GOt here");
	//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, inp.mi.mouseData, 0);
	SendInput(1, &inp, sizeof(inp));
}
void RemoteDesktop::Server::_Handle_File(RemoteDesktop::Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	File_Header fh;
	assert(header->PayloadLen > sizeof(fh));
	memcpy(&fh, data, sizeof(fh));
	data += sizeof(fh);
	std::string fname(fh.RelativePath);
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;

	DEBUG_MSG("% BEG FILE: %", path.size(), path);
	sh->WriteToFile(path, data, fh.ChunkSize, fh.Last);
	//int openoptions = std::ios::binary;
	//if (fh.ID == 0) openoptions |= std::ios::trunc;// erase everything in the file 
	//else openoptions |= std::ios::app;// append data 
	//std::ofstream f(path, openoptions);
	//f.write();

	DEBUG_MSG("% END FILE: %", path.size(), path);
}

void RemoteDesktop::Server::_Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	unsigned char size = (unsigned char)*data;
	data++;
	std::string fname(data, size);
	data += size;
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;
	DEBUG_MSG("% BEG FOLDER: %", path.size(), path);
	CreateDirectoryA(path.c_str(), NULL);
	DEBUG_MSG("% END FOLDER: %", path.size(), path);
}
void RemoteDesktop::Server::_Handle_ConnectionRequest(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	sh->Authorized = false;
	assert(header->PayloadLen == sizeof(sh->Connection_Info));
	memcpy(&sh->Connection_Info, data, sizeof(sh->Connection_Info));
	auto uname = std::wstring(sh->Connection_Info.full_name);
	if (uname.size() > 2){
		DEBUG_MSG("Connect Request from %", ws2s(uname));
		//EventLog::WriteLog(msgs, EventLog::EventType::INFORMATIONAL, EventLog::EventCategory::NETWORK_CATEGORY, EventLog::EventID::DISCONNECT);
		//EventLog::WriteLog(uname + L" is attempted to connect to this machine");
		std::wstring lastauthuser = GetLast_UserConnectName();
		//close the gateway dialog in any case
		auto a = _GatewayConnect_Dialog;
		if (a) a->Close();

		if (lastauthuser == uname){//this is the last user that connected, allow through likely a reconnect
			sh->Authorized = true;
			_NewClients.push_back(sh);
		}
		else {// show pop up asking for permission to connect
			_NewConnect_Dialog = std::make_shared<NewConnect_Dialog>();
			_NewConnect_Dialog->Show(uname);
			_NewConnect_Dialog->OnAllow = DELEGATE(&RemoteDesktop::Server::_OnAllowConnection);
			_NewConnect_Dialog->OnDeny = DELEGATE(&RemoteDesktop::Server::_OnDenyConnection);
		}
	}
	else {
		DEBUG_MSG("Setting user to disconnected because there was no name sent with the access request.");
		sh->Disconnect();
	}
}
#include "..\RemoteDesktop_Library\ProcessUtils.h"

void RemoteDesktop::Server::_Handle_ElevateProcess(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	Elevate_Header h;
	assert(header->PayloadLen == sizeof(h));
	memcpy(&h, data, sizeof(h));
	HANDLE token = NULL;
	std::wstring user(h.Username);
	std::wstring pass(h.Password);
	if (user.size() < 2 || pass.size() < 2) return;
	auto splits = split(user, L'\\');
	bool authenticated = false;
	if (splits.size() == 2){
		authenticated = LogonUser(splits[1].c_str(), splits[0].c_str(), h.Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token);
	}
	else {
		authenticated = LogonUser(h.Username, NULL, h.Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token);
	}

	auto isadmin = IsUserAdmin(token);
	if (!isadmin && splits.size() == 2){//check domain controller for admin group
		isadmin = IsUserAdmin(user);
	}	

	if (isadmin && !IsElevated()) {
		wchar_t szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
		wchar_t cmndargs[] = L" -delayed_run";
		if (splits.size() == 2){
			if (LaunchProcess(szPath, cmndargs, splits[1].c_str(), splits[0].c_str(), h.Password, token)) _TriggerShutDown();//this will cause this process to shut down 
		}
		else {
			if (LaunchProcess(szPath, cmndargs, h.Username, NULL, h.Password, token)) _TriggerShutDown();//this will cause this process to shut down 
		}
	}
	if (token != nullptr) CloseHandle(token);
	DEBUG_MSG("Got here");
}

void RemoteDesktop::Server::OnReceive(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh) {
	if (!sh->Authorized){//if the user is not authorized yet, only allow connect requests
		if (header->Packet_Type == NetworkMessages::CONNECT_REQUEST) return _Handle_ConnectionRequest(header, data, sh);
		return;// dont process
	}//else let through the other packet receives
	switch (header->Packet_Type){
	case NetworkMessages::KEYEVENT:
		_HandleKeyEvent(header, data, sh);
		break;
	case NetworkMessages::MOUSEEVENT:
		_Handle_MouseUpdate(header, data, sh);
		break;
	case NetworkMessages::CAD:
		SetEvent(_CADEventHandle.get());
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
	case NetworkMessages::SETTINGS:
		_Handle_Settings(header, data, sh);
		break;
	case NetworkMessages::ELEVATEPROCESS:
		_Handle_ElevateProcess(header, data, sh);
		break;

	default:
		break;
	}
}
void RemoteDesktop::Server::OnDisconnect(std::shared_ptr<RemoteDesktop::SocketHandler>& sh) {
	if (!sh) return;
	if (!sh->Authorized) return;//dont care about non authorized disconnects
	SetLast_UserConnectName(L"");//set this to empty because the user will have to be allowed next connect attempt
	auto usname = std::wstring(sh->Connection_Info.full_name);
	if (usname.size() > 2){
		std::vector<std::wstring> msgs;
		msgs.push_back(usname + L" has Disconnected from this machine. Details Below:");
		msgs.push_back(std::wstring(L"Computer: ") + std::wstring(sh->Connection_Info.computername));
		msgs.push_back(std::wstring(L"Domain: ") + std::wstring(sh->Connection_Info.domain));
		msgs.push_back(std::wstring(L"Full Name: ") + std::wstring(sh->Connection_Info.full_name));
		msgs.push_back(std::wstring(L"IP Address: ") + std::wstring(sh->Connection_Info.ip_addr));
		EventLog::WriteLog(msgs, EventLog::EventType::INFORMATIONAL, EventLog::EventCategory::NETWORK_CATEGORY, EventLog::EventID::DISCONNECT);
		auto con = usname + L" has Disconnected from your machine . . .";
		_SystemTray->Popup(L"Connection Disconnected", con.c_str(), 2000);
	}
}

void RemoteDesktop::Server::_HandleNewClients(Screen& screen, int index, std::vector<std::shared_ptr<SocketHandler>>& newclients){
	if (newclients.empty()) return;
	auto sendimg = screen.Image->Clone();
	sendimg.Compress();
	NetworkMsg msg;
	New_Image_Header h;
	h.YOffset = screen.YOffset;
	h.XOffset = screen.XOffset;
	h.Index = index;
	h.Height = screen.Image->Height;
	h.Width = screen.Image->Width;
	DEBUG_MSG("Servicing new Client %,  %, %, %", index, sendimg.Height, sendimg.Width, sendimg.size_in_bytes());

	msg.push_back(h);
	msg.data.push_back(DataPackage((char*)sendimg.get_Data(), sendimg.size_in_bytes()));

	for (size_t i = 0; i < newclients.size(); i++){
		auto a(newclients[i]);
		if (a) a->Send(NetworkMessages::RESOLUTIONCHANGE, msg);
	}
}
bool RemoteDesktop::Server::_HandleResolutionUpdates(Screen& screen, Screen& lastscreen, int index) {
	bool reschange = (screen.Image->Height != lastscreen.Image->Height || screen.Image->Width != lastscreen.Image->Width) && (screen.Image->Height > 0 && lastscreen.Image->Width > 0);
	//if there was a resolution change
	if (reschange){
		std::vector<char> tmpbuf;
		auto sendimg = screen.Image->Clone();
		sendimg.Compress();
		NetworkMsg msg;
		New_Image_Header h;
		h.YOffset = screen.YOffset;
		h.XOffset = screen.XOffset;
		h.Index = index;
		h.Height = screen.Image->Height;
		h.Width = screen.Image->Width;

		msg.push_back(h);
		msg.data.push_back(DataPackage((char*)sendimg.get_Data(), sendimg.size_in_bytes()));
		_NetworkServer->Send(NetworkMessages::RESOLUTIONCHANGE, msg, INetwork::Auth_Types::AUTHORIZED);
		return true;
	}
	return false;
}


void RemoteDesktop::Server::_Handle_ScreenUpdates(Image& img, Rect& rect, int index){
	if (rect.width > 0 && rect.height > 0){

		NetworkMsg msg;
		auto imgdif = Image::Copy(img, rect);
		imgdif.Compress();
		Update_Image_Header h;
		h.rect = rect;
		h.Index = index;

		msg.push_back(h);
		msg.data.push_back(DataPackage((char*)imgdif.get_Data(), imgdif.size_in_bytes()));

		//DEBUG_MSG("_Handle_ScreenUpdates %, %, %", rect.height, rect.width, imgdif.size_in_bytes);
		_NetworkServer->Send(NetworkMessages::UPDATEREGION, msg, INetwork::Auth_Types::AUTHORIZED);
	}
}
void RemoteDesktop::Server::_Handle_UAC_Permission(){
	_NetworkServer->Send(NetworkMessages::UAC_BLOCKED, INetwork::Auth_Types::AUTHORIZED);
}
void RemoteDesktop::Server::_Handle_MouseUpdates(const std::unique_ptr<MouseCapture>& mousecapturing){
	static auto begintimer = std::chrono::high_resolution_clock::now();

	mousecapturing->Update();
	if (mousecapturing->Last_ScreenPos != mousecapturing->Current_ScreenPos || mousecapturing->Last_Mouse != mousecapturing->Current_Mouse){//mouse pos is different
		//if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begintimer).count() < 50) return;
		//	begintimer = std::chrono::high_resolution_clock::now();

		NetworkMsg msg;
		MouseEvent_Header h;
		h.pos = mousecapturing->Current_ScreenPos;
		h.HandleID = mousecapturing->Current_Mouse;
		msg.push_back(h);
		_NetworkServer->Send(NetworkMessages::MOUSEEVENT, msg, INetwork::Auth_Types::AUTHORIZED);
		if (mousecapturing->Last_Mouse != mousecapturing->Current_Mouse) DEBUG_MSG("Sending mouse Iconchange %", mousecapturing->Current_Mouse);
		mousecapturing->Last_ScreenPos = mousecapturing->Current_ScreenPos;
		mousecapturing->Last_Mouse = mousecapturing->Current_Mouse;

	}
}
void RemoteDesktop::Server::OnConnect(std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	std::lock_guard<std::mutex> lock(_ClientLock);
	_PendingNewClients.push_back(sh);
	sh->Authorized = false;
	DEBUG_MSG("New Client OnConnect");
}
void RemoteDesktop::Server::_ShowGatewayDialog(int id){
	_GatewayConnect_Dialog = std::make_shared<GatewayConnect_Dialog>();
	_GatewayConnect_Dialog->Show(id);
}
void RemoteDesktop::Server::Listen(std::wstring port, std::wstring host){
	_NetworkServer = std::make_unique<Network_Server>();
	_NetworkServer->OnConnected = std::bind(&RemoteDesktop::Server::OnConnect, this, std::placeholders::_1);
	_NetworkServer->OnReceived = std::bind(&RemoteDesktop::Server::OnReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_NetworkServer->OnDisconnect = std::bind(&RemoteDesktop::Server::OnDisconnect, this, std::placeholders::_1);
	_NetworkServer->Start(port, host);
	_Run();
}
void RemoteDesktop::Server::ReverseConnect(std::wstring port, std::wstring host, std::wstring gatewayurl){
	_NetworkServer = std::make_unique<Network_Client>();
	_NetworkServer->OnConnected = std::bind(&RemoteDesktop::Server::OnConnect, this, std::placeholders::_1);
	_NetworkServer->OnReceived = std::bind(&RemoteDesktop::Server::OnReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_NetworkServer->OnDisconnect = std::bind(&RemoteDesktop::Server::OnDisconnect, this, std::placeholders::_1);
	auto ptr = (Network_Client*)_NetworkServer.get();

	ptr->OnGatewayConnected = std::bind(&RemoteDesktop::Server::_ShowGatewayDialog, this, std::placeholders::_1);
	ptr->Start(port, host, gatewayurl);
	_Run();
}

void RemoteDesktop::Server::_Run() {

	//switch to input desktop 
	_DesktopMonitor->Switch_to_Desktop(DesktopMonitor::Desktops::INPUT);

	_VirtualScreen->Update();

	auto shutdownhandle(RAIIHANDLE(OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\SessionEventRDProgram")));

	WaitForSingleObject(shutdownhandle.get(), 100);//call this to get the first signal from the service

	DWORD dwEvent;
	auto lastwaittime = FRAME_CAPTURE_INTERVAL;
	std::vector<std::shared_ptr<SocketHandler>> tmpbuffer;

	while (_NetworkServer->Is_Running()){
		if (shutdownhandle.get() == NULL) std::this_thread::sleep_for(std::chrono::milliseconds(lastwaittime));//sleep
		else {
			dwEvent = WaitForSingleObject(shutdownhandle.get(), lastwaittime);
			if (dwEvent == 0){
				_NetworkServer->Stop(false);//stop program!
				break;
			}
		}
		auto t1 = Timer(true);

		if (_NetworkServer->Connection_Count() <= 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));//sleep if there are no clients connected.
			continue;
		}

		if (!_DesktopMonitor->Is_InputDesktopSelected()) {
			if (!_DesktopMonitor->Switch_to_Desktop(DesktopMonitor::Desktops::INPUT)){
				_Handle_UAC_Permission();
			}
		}

		_Handle_MouseUpdates(mousecapturing);

		_VirtualScreen->Update();
		{
			std::lock_guard<std::mutex> lock(_ClientLock);
			for (size_t i = 0; i < _NewClients.size(); i++) tmpbuffer.push_back(_NewClients[i]);
			_NewClients.clear();
		}
		for (auto& a : tmpbuffer){
			std::wstring name = a->Connection_Info.full_name;
			std::vector<std::wstring> msgs;
			msgs.push_back(name + L" has Connected to this machine. Details Below:");
			msgs.push_back(std::wstring(L"Computer: ") + std::wstring(a->Connection_Info.computername));
			msgs.push_back(std::wstring(L"Domain: ") + std::wstring(a->Connection_Info.domain));
			msgs.push_back(std::wstring(L"Full Name: ") + std::wstring(a->Connection_Info.full_name));
			msgs.push_back(std::wstring(L"IP Address: ") + std::wstring(a->Connection_Info.ip_addr));
			EventLog::WriteLog(msgs, EventLog::EventType::INFORMATIONAL, EventLog::EventCategory::NETWORK_CATEGORY, EventLog::EventID::CONNECT);
			name += L" has connected to your computer . . . ";
			if (name.size()>2) _SystemTray->Popup(L"New Connection Established", name.c_str(), 10000);
		}

		auto m = min(_VirtualScreen->Current.size(), _VirtualScreen->Previous.size());
		for (size_t i = 0; i < m; i++){
			_HandleNewClients(_VirtualScreen->Current[i], i, tmpbuffer);
			if (!_HandleResolutionUpdates(_VirtualScreen->Current[i], _VirtualScreen->Previous[i], i)){
				auto rect = Image::Difference(*_VirtualScreen->Previous[i].Image, *_VirtualScreen->Current[i].Image);
				_Handle_ScreenUpdates(*_VirtualScreen->Current[i].Image, rect, i);
			}
		}
		tmpbuffer.clear();//make sure to clear the new clienrts
		t1.Stop();
		auto tim = (int)t1.Elapsed_milli();
		lastwaittime = FRAME_CAPTURE_INTERVAL - tim;
		if (lastwaittime < 0) lastwaittime = 0;
		//DEBUG_MSG("Time for work... %", t1.Elapsed_milli());
	}
	DEBUG_MSG("Stopping Main Server Loop");
	_NetworkServer = nullptr;
}
