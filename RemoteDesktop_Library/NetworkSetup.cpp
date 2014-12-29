#include "stdafx.h"
#include "NetworkSetup.h"
#include <thread>
#include <Iphlpapi.h>
#include "..\RemoteDesktop_Library\WinHttpClient.h"
#include "Lmcons.h"
#include "Handle_Wrapper.h"
#include "Desktop_Monitor.h"

bool RemoteDesktop::_INTERNAL::NetworkStarted = false;

bool RemoteDesktop::StartupNetwork(){
	if (RemoteDesktop::_INTERNAL::NetworkStarted) return RemoteDesktop::_INTERNAL::NetworkStarted;
	WSADATA wsaData = { 0 };
	RemoteDesktop::_INTERNAL::NetworkStarted = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
	return RemoteDesktop::_INTERNAL::NetworkStarted;
}
void RemoteDesktop::ShutDownNetwork(){
	WSACleanup();
	RemoteDesktop::_INTERNAL::NetworkStarted = false;
}
void RemoteDesktop::PrimeNetwork(unsigned short int port){
	if (!StartupNetwork()) return;

	SOCKET listensocket = INVALID_SOCKET;

	listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listensocket == INVALID_SOCKET) return;
	RemoteDesktop::StandardSocketSetup(listensocket);
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(port);
	service.sin_addr.s_addr = INADDR_ANY;

	if (bind(listensocket, (SOCKADDR *)& service, sizeof(service)) == 0) {
		std::chrono::milliseconds dura(200);
		std::this_thread::sleep_for(dura);
		closesocket(listensocket);
		std::this_thread::sleep_for(dura);
	}
}
void RemoteDesktop::StandardSocketSetup(SOCKET socket){
	//set to non blocking
	u_long iMode = 1;
	ioctlsocket(socket, FIONBIO, &iMode);
	//set no delay 
	BOOL nodly = TRUE;
	if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nodly, sizeof(nodly)) == SOCKET_ERROR){
		auto errmsg = WSAGetLastError();
		DEBUG_MSG("failed to sent TCP_NODELY with error = %", errmsg);
	}
}
SOCKET RemoteDesktop::Connect(std::wstring port, std::wstring host){
	if (!StartupNetwork()) return INVALID_SOCKET;
	std::chrono::milliseconds dura(1000);
	std::this_thread::sleep_for(dura);
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfoW *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port

	auto iResult = GetAddrInfoW(host.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0) return INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) return INVALID_SOCKET;
		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			DEBUG_MSG("Server Down....");
			continue;
		}
		else DEBUG_MSG("Connect Success!");
		break;
	}

	FreeAddrInfoW(result);

	if (ConnectSocket == INVALID_SOCKET) return INVALID_SOCKET;
	StandardSocketSetup(ConnectSocket);

	auto newevent(RAIIHANDLE(WSACreateEvent()));

	WSAEventSelect(ConnectSocket, newevent.get(), FD_CONNECT);
	auto Index = WaitForSingleObject(newevent.get(), 1000);
	WSANETWORKEVENTS NetworkEvents;
	WSAEnumNetworkEvents(ConnectSocket, newevent.get(), &NetworkEvents);

	if ((Index == WSA_WAIT_FAILED) || (Index == WSA_WAIT_TIMEOUT)) {
		DEBUG_MSG("Connect Failed!");
		return INVALID_SOCKET;
	}
	return ConnectSocket;
}
std::string RemoteDesktop::GetMAC(){

	PIP_ADAPTER_INFO AdapterInfo;
	DWORD dwBufLen = sizeof(AdapterInfo);

	AdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen     variable
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(AdapterInfo);
		AdapterInfo = (IP_ADAPTER_INFO *)malloc(dwBufLen);
	}
	char mac[24];
	std::string macs;
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
		do {
			if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET){//get an ethernet interface
				
				sprintf_s(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
					pAdapterInfo->Address[0], pAdapterInfo->Address[1],
					pAdapterInfo->Address[2], pAdapterInfo->Address[3],
					pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
				macs= std::string(mac);
				break;
			}
			pAdapterInfo = pAdapterInfo->Next;
		} while (pAdapterInfo);
	}
	free(AdapterInfo);
	return macs;
}
int GetFileCreateTime()//used for randomness in session 
{
	wchar_t szPath[MAX_PATH];
	bool ret = false;
	if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) return 0;

	auto hFile(RAIIHANDLE(CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)));

	if (hFile.get() == INVALID_HANDLE_VALUE) return 0;

	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;


	// Retrieve the file times for the file.
	if (!GetFileTime(hFile.get(), &ftCreate, &ftAccess, &ftWrite))
		return 0;


	FileTimeToSystemTime(&ftCreate, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	return (int)stLocal.wMonth + (int)stLocal.wDay + (int)stLocal.wYear + (int)stLocal.wHour + (int)stLocal.wMinute + (int)stLocal.wSecond + (int)stLocal.wMilliseconds;//this is just some randomness to add into the mix

}

int RemoteDesktop::GetProxyID(std::wstring url, std::wstring& aeskey){

	char comp[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameA(comp, &len);
	std::string computername(comp);

	std::string username = RemoteDesktop::DesktopMonitor::get_ActiveUser();

	auto mac = GetMAC();
	std::string adddata = "computername=" + computername + "&username=" + username + "&mac=" + mac + "&session=" + std::to_string(GetFileCreateTime());
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
	auto id = -1;
	auto httpResponseContent = cl.GetResponseContent();
	if (httpResponseContent.size() > 0){
		auto splits = split(httpResponseContent, L'\n');
		if (splits.size() == 2){
			id = std::stoi(splits[0]);
			aeskey = splits[1];
		}
	}
	return id;
}

