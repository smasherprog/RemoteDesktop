#ifndef NETWORKSETUP_H
#define NETWORKSETUP_H
#include <string>
#include <memory>
#include "CommonNetwork.h"

namespace RemoteDesktop{

	class SocketHandler;

	namespace _INTERNAL{
		extern bool NetworkStarted;
	}
	bool StartupNetwork();
	void ShutDownNetwork();
	void AddFirewallException();
	void RemoveFirewallException();
	bool TryToElevate(LPWSTR* argv, int argc);
	SOCKET Connect(std::wstring port, std::wstring host);
	void StandardSocketSetup(SOCKET socket);
	std::string GetMAC();
	RemoteDesktop::Network_Return SendLoop(SocketHandler* sock, char* data, int len);
	RemoteDesktop::Network_Return ReceiveLoop(SocketHandler* sock, std::vector<char>& outdata, int& datareceived);

}
#endif