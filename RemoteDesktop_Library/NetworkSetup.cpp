#include "stdafx.h"
#include "NetworkSetup.h"
#include <thread>
#include <Iphlpapi.h>

#include "Lmcons.h"
#include "Handle_Wrapper.h"
#include "Desktop_Monitor.h"
#include "Firewall.h"
#include "Shellapi.h"

bool RemoteDesktop::_INTERNAL::NetworkStarted = false;

bool RemoteDesktop::StartupNetwork(){
	if (RemoteDesktop::_INTERNAL::NetworkStarted) return RemoteDesktop::_INTERNAL::NetworkStarted;
	WSADATA wsaData = { 0 };
	RemoteDesktop::_INTERNAL::NetworkStarted = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
	return RemoteDesktop::_INTERNAL::NetworkStarted;
}
void RemoteDesktop::ShutDownNetwork(){
	if (RemoteDesktop::_INTERNAL::NetworkStarted) WSACleanup();
	RemoteDesktop::_INTERNAL::NetworkStarted = false;
}
BOOL IsAppRunningAsAdminMode()
{
	BOOL fIsRunAsAdmin = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	PSID pAdministratorsGroup = NULL;

	// Allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdministratorsGroup))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

	// Determine whether the SID of administrators group is enabled in 
	// the primary access token of the process.
	if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (pAdministratorsGroup)
	{
		FreeSid(pAdministratorsGroup);
		pAdministratorsGroup = NULL;
	}

	// Throw the error if something failed in the function.
	if (ERROR_SUCCESS != dwError)
	{
		throw dwError;
	}

	return fIsRunAsAdmin;
}
BOOL IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}
#include <lm.h>
bool IsUserAdmin(){


	int found;
	DWORD i, l;
	HANDLE hTok;
	PSID pAdminSid;
	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;

	byte rawGroupList[4096];
	TOKEN_GROUPS& groupList = *((TOKEN_GROUPS *)rawGroupList);

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hTok))
	{
		printf("Cannot open thread token, trying process token [%lu].\n",
			GetLastError());
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok))
		{
			printf("Cannot open process token, quitting [%lu].\n",
				GetLastError());
			return false;
		}
	}

	// normally, I should get the size of the group list first, but ...
	l = sizeof(rawGroupList);
	if (!GetTokenInformation(hTok, TokenGroups, &groupList, l, &l))
	{
		printf("Cannot get group list from token [%lu].\n",
			GetLastError());
		return false;
	}

	// here, we cobble up a SID for the Administrators group, to compare to.
	if (!AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid))
	{
		printf("Cannot create SID for Administrators [%lu].\n",
			GetLastError());
		return false;
	}

	// now, loop through groups in token and compare
	found = 0;
	for (i = 0; i < groupList.GroupCount; ++i)
	{
		if (EqualSid(pAdminSid, groupList.Groups[i].Sid))
		{
			found = 1;
			break;
		}
	}

	FreeSid(pAdminSid);
	CloseHandle(hTok);
	if (!found){//search domain groups as well
		DWORD rc;
		wchar_t user_name[256];
		USER_INFO_1 *info;
		DWORD size = sizeof(user_name);

		GetUserNameW(user_name, &size);

		rc = NetUserGetInfo(NULL, user_name, 1, (byte **)&info);
		if (rc != NERR_Success)
			return false;

		found = info->usri1_priv == USER_PRIV_ADMIN;
		NetApiBufferFree(info);
	}
	return found;
}

bool RemoteDesktop::TryToElevate(LPWSTR* argv, int argc){
	if (IsElevated()) return false;//allready elevated
	if (!IsUserAdmin()) return false;// cannot elevate process anyway
	//if (IsAppRunningAsAdminMode()) return false;//application is running as admin
	//we have the power.. TRY TO ELVATE!!!

	// Launch itself as admin
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	std::wstring args;
	for (auto i = 1; i < argc; i++) {//ignore the first arg, its the app name
		args += L" ";
		args += argv[i];
	}
	sei.lpVerb = L"runas";
	sei.lpFile = argv[0];//first arg is the full path to the exe
	sei.lpParameters = args.c_str();
	sei.hwnd = NULL;
	sei.nShow = SW_NORMAL;
	ShellExecuteEx(&sei);
	return true;//
}

void RemoteDesktop::AddFirewallException(){

	WindowsFirewall firewall;

	TCHAR szModuleName[MAX_PATH];
	GetModuleFileName(NULL, szModuleName, MAX_PATH);
	firewall.AddProgramException(szModuleName, L"RAT Gateway Tool");

	std::chrono::milliseconds dura(200);
	std::this_thread::sleep_for(dura);
}
//easier to add a remove via the command line
void RemoteDesktop::RemoveFirewallException(){

	WindowsFirewall firewall;

	TCHAR szModuleName[MAX_PATH];
	GetModuleFileName(NULL, szModuleName, MAX_PATH);
	firewall.RemoveProgramException(szModuleName, L"RAT Gateway Tool");

	std::chrono::milliseconds dura(200);
	std::this_thread::sleep_for(dura);

}


void RemoteDesktop::StandardSocketSetup(SOCKET socket){
	//set to non blocking
	u_long iMode = 1;
	ioctlsocket(socket, FIONBIO, &iMode);
	int optLen = sizeof(int);
	int optVal = 64 * 1024;
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, optLen);
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
				macs = std::string(mac);
				break;
			}
			pAdapterInfo = pAdapterInfo->Next;
		} while (pAdapterInfo);
	}
	free(AdapterInfo);
	return macs;
}