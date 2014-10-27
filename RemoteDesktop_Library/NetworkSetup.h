#ifndef NETWORKSETUP_H
#define NETWORKSETUP_H

namespace RemoteDesktop{
	namespace _INTERNAL{
		extern bool NetworkStarted;
	}
	bool StartupNetwork();
	void ShutDownNetwork();
}
#endif