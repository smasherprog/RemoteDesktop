#include "stdafx.h"
#include "RD_Server.h"
#include "ScreenCapture.h"
#include "MouseCapture.h"
#include "Desktop_Monitor.h"
#include "BaseServer.h"
#include "SocketHandler.h"
#include "Rect.h"
#include "CommonNetwork.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\Clipboard_Monitor.h"
#include "..\RemoteDesktop_Library\Delegate.h"
#include "..\RemoteDesktopServer_Library\SystemTray.h"
#include "..\RemoteDesktop_Library\WinHttpClient.h"
#include "Lmcons.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"
#include "..\RemoteDesktop_Library\Utilities.h"

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

RemoteDesktop::RD_Server::RD_Server() :
_CADEventHandle(OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEventRDCad")),
_SelfRemoveEventHandle(OpenEvent(EVENT_MODIFY_STATE, FALSE, L"Global\\SessionEventRemoveSelf"))
{

#if _DEBUG
	_DebugConsole = std::make_unique<CConsole>();
#endif

	DWORD bufsize = 256;
	TCHAR buff[256];
	GetUserName(buff, &bufsize);
	std::wstring uname = buff;
	_RunningAsService = find_substr(uname, std::wstring(L"system")) != -1;

	mousecapturing = std::make_unique<MouseCapture>();
	_DesktopMonitor = std::make_unique<DesktopMonitor>();
	_NetworkServer = std::make_unique<BaseServer>(
		DELEGATE(&RemoteDesktop::RD_Server::OnConnect, this),
		DELEGATE(&RemoteDesktop::RD_Server::OnReceive, this),
		DELEGATE(&RemoteDesktop::RD_Server::OnDisconnect, this));
	_ScreenCapture = std::make_unique<ScreenCapture>();
	_ClipboardMonitor = std::make_unique<ClipboardMonitor>(DELEGATE(&RemoteDesktop::RD_Server::_OnClipboardChanged, this));
	_SystemTray = std::make_unique<SystemTray>();
	_SystemTray->Start();
}
RemoteDesktop::RD_Server::~RD_Server(){
	if (_RemoveOnExit) {
		if (_SelfRemoveEventHandle.get_Handle() != nullptr) {
			SetEvent(_SelfRemoveEventHandle.get_Handle());//signal the self removal process 
		}
		else DeleteMe();//try a self removal 
	}
}
void RemoteDesktop::RD_Server::_Handle_DisconnectandRemove(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	_RemoveOnExit = true;

	_NetworkServer->GracefulStop();//this will cause the main loop to stop and the program to exit
}
void RemoteDesktop::RD_Server::_Handle_ImageSettings(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	int q = 70;
	char g;
	memcpy(&q, data, sizeof(q));
	data += sizeof(q);
	memcpy(&g, data, sizeof(g));
	Image_Settings::GrazyScale = g == 1 ? true : false;
	Image_Settings::Quality = q;
	DEBUG_MSG("Setting Quality to % and GrayScale to %", q, g);
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
	unsigned char size = (unsigned char)*data;
	data++;
	std::string fname(data, size);
	data += size;
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;
	int isize = 0;
	memcpy(&isize, data, sizeof(isize));
	data += sizeof(isize);
	DEBUG_MSG("% BEG FILE: %", path.size(), path);
	std::ofstream f(path, std::ios::binary);
	f.write(data, isize);
	DEBUG_MSG("% END FILE: %", path.size(), path);
	if (find_substr(path, std::string(".dll")) != -1){
		DEBUG_MSG("found it");
	}
}

void RemoteDesktop::RD_Server::_Handle_Folder(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	unsigned char size = (unsigned char)*data;
	data++;
	std::string fname(data, size);
	data += size;
	std::string path = "c:\\users\\" + _DesktopMonitor->get_ActiveUser() + "\\desktop\\" + fname;
	DEBUG_MSG("% BEG FOLDER: %", path.size(), path);
	CreateDirectoryA(path.c_str(), NULL);
	DEBUG_MSG("% END FOLDER: %", path.size(), path);
}
void RemoteDesktop::RD_Server::_Handle_ConnectionInfo(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh){
	ConnectionInfo_Header h;
	assert(header->PayloadLen == sizeof(h));
	memcpy(&h, data, header->PayloadLen);
	h.UserName[UNAMELEN] = 0;
	sh->UserName = std::wstring(h.UserName);
	auto con = sh->UserName + L" has connected to your machine . . .";
	_SystemTray->Popup(L"Connection Established", con.c_str(), 2000);
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
		SetEvent(_CADEventHandle.get_Handle());
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
	case NetworkMessages::IMAGESETTINGS:
		_Handle_ImageSettings(header, data, sh);
		break;
	case NetworkMessages::CONNECTIONINFO:
		_Handle_ConnectionInfo(header, data, sh);
		break;

	default:
		break;
	}
}
void RemoteDesktop::RD_Server::OnDisconnect(std::shared_ptr<SocketHandler>& sh) {
	auto con = sh->UserName + L" has Disconnected from your machine . . .";
	_SystemTray->Popup(L"Connection Disconnected", con.c_str(), 2000);
}

void RemoteDesktop::RD_Server::_HandleNewClients(Image& imgg){
	if (_NewClients.empty()) return;
	std::vector<char> tmpbuf;
	auto sendimg = imgg.Clone(tmpbuf);
	sendimg.Compress();
	NetworkMsg msg;
	int sz[2];
	sz[0] = sendimg.height;
	sz[1] = sendimg.width;
	DEBUG_MSG("Servicing new Client %, %, %", sendimg.height, sendimg.width, sendimg.size_in_bytes);
	msg.data.push_back(DataPackage((char*)&sz, sizeof(int) * 2));
	msg.data.push_back(DataPackage((char*)sendimg.data, sendimg.size_in_bytes));

	std::lock_guard<std::mutex> lock(_NewClientLock);

	for (auto& a : _NewClients){

		a->Send(NetworkMessages::RESOLUTIONCHANGE, msg);
	}
	_NewClients.clear();
}
bool RemoteDesktop::RD_Server::_HandleResolutionUpdates(Image& imgg, Image& _lastimg){
	bool reschange = (imgg.height != _lastimg.height || imgg.width != _lastimg.width) && (imgg.height > 0 && _lastimg.width > 0);
	//if there was a resolution change
	if (reschange){
		std::vector<char> tmpbuf;
		auto sendimg = imgg.Clone(tmpbuf);
		sendimg.Compress();
		NetworkMsg msg;
		int sz[2];
		sz[0] = sendimg.height;
		sz[1] = sendimg.width;

		msg.data.push_back(DataPackage((char*)&sz, sizeof(int) * 2));
		msg.data.push_back(DataPackage((char*)sendimg.data, sendimg.size_in_bytes));
		_NetworkServer->SendToAll(NetworkMessages::RESOLUTIONCHANGE, msg);
		return true;
	}
	return false;
}


void RemoteDesktop::RD_Server::_Handle_ScreenUpdates(Image& img, Rect& rect, std::vector<unsigned char>& buffer){
	if (rect.width > 0 && rect.height > 0){

		NetworkMsg msg;
		auto imgdif = Image::Copy(img, rect, buffer);
		imgdif.Compress();
		msg.push_back(rect);
		msg.data.push_back(DataPackage((char*)imgdif.data, imgdif.size_in_bytes));

		//DEBUG_MSG("_Handle_ScreenUpdates %, %, %", rect.height, rect.width, imgdif.size_in_bytes);
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
void RemoteDesktop::RD_Server::_ShowConnectID(int id){
	auto msg = std::wstring(L"Please give this number to your technician: ");
	msg += std::to_wstring(id);
	auto ret = MessageBox(
		NULL,
		msg.c_str(),
		(LPCWSTR)L"Connection ID",
		MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION
		);
}
void RemoteDesktop::RD_Server::Listen(unsigned short port, std::wstring host, std::wstring proxy) {
	std::thread msgdialog;
	if (proxy.size() > 1){
		std::wstring aes;
		GetProxyID(proxy, aes);
		if (_NetworkServer->ProxyHeader.Src_Id == -1) return;
		_NetworkServer->StartListening(port, host, aes);
		msgdialog = std::thread(&RD_Server::_ShowConnectID, this, _NetworkServer->ProxyHeader.Src_Id);
	}
	else {
		_NetworkServer->StartListening(port, host);
	}

	std::vector<unsigned char> curimagebuffer;
	std::vector<unsigned char> lastimagebuffer;
	std::vector<unsigned char> sendimagebuffer;

	Image _LastImage = _ScreenCapture->GetPrimary(lastimagebuffer);//<---Seed the image
	auto pingpong = true;
	RAIIHANDLE shutdownhandle(OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\SessionEventRDProgram"));

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

		if (!_DesktopMonitor->Is_InputDesktopSelected())
		{
			_ScreenCapture->ReleaseHandles();//cannot have lingering handles to the exisiting desktop
			_DesktopMonitor->Switch_to_Desktop(DesktopMonitor::Desktops::INPUT);
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

bool RemoteDesktop::RD_Server::GetProxyID(std::wstring url, std::wstring& aeskey){
	_NetworkServer->ProxyHeader.Src_Id = _NetworkServer->ProxyHeader.Dst_Id= - 1;
	char comp[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameA(comp, &len);
	std::string computername(comp);

	char user[UNLEN + 1];
	len = UNLEN + 1;
	GetUserNameA(user, &len);
	std::string username(user);
	auto mac = GetMAC();
	std::string adddata = "computername=" + computername + "&username=" + username + "&mac=" + mac;
	WinHttpClient cl(url.c_str());

	cl.SetAdditionalDataToSend((BYTE*)adddata.c_str(), adddata.size());

	// Set request headers.
	wchar_t szSize[50] = L"";
	swprintf_s(szSize, L"%d", adddata.size());
	std::wstring headers = L"Content-Length: ";
	headers += szSize;
	headers += L"\r\nContent-Type: application/x-www-form-urlencoded\r\n";
	cl.SetAdditionalRequestHeaders(headers);

	cl.SendHttpRequest(L"POST");
	auto httpResponseContent = cl.GetResponseContent();
	if (httpResponseContent.size() > 0){
		auto splits = split(httpResponseContent, L'\n');
		if (splits.size() == 2){
			_NetworkServer->ProxyHeader.Src_Id = std::stoi(splits[0]);
			aeskey = splits[1];
			return true;
		}
		else return false;
	}
	return false;
}
