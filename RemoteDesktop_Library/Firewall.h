#ifndef FIREWALL123_H
#define FIREWALL123_H
#include <string>
#include <netfw.h>

namespace RemoteDesktop{
	class WindowsFirewall{
		HRESULT comInit = E_FAIL;
		INetFwProfile* FirewallProfile = nullptr;

	public:
		WindowsFirewall();
		~WindowsFirewall();

		bool AddProgramException(std::wstring exefullpath, std::wstring name);
		bool IsProgamEnabled(std::wstring exefullpath);
	};
}

#endif