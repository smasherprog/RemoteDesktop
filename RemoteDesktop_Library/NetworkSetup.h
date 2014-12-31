#ifndef NETWORKSETUP_H
#define NETWORKSETUP_H
#include <string>

namespace RemoteDesktop{
	namespace _INTERNAL{
		extern bool NetworkStarted;
	}
	bool StartupNetwork();
	void ShutDownNetwork();
	void PrimeNetwork(unsigned short int port);
	SOCKET Connect(std::wstring port, std::wstring host);
	void StandardSocketSetup(SOCKET socket);
	std::string GetMAC();

}
#endif