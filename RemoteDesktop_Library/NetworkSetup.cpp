#include "stdafx.h"
#include "NetworkSetup.h"

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