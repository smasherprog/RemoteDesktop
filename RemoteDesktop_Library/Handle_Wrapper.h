#ifndef RAIIHDESKTOP123_h
#define RAIIHDESKTOP123_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace RemoteDesktop{
	template<typename H, BOOL(WINAPI *Releaser)(H)>
	class Handle
	{
	private:
		H m_handle = nullptr;

	public:
		Handle(H handle) : m_handle(handle) { }
		~Handle() { if (m_handle != nullptr) (*Releaser)(m_handle); }
		H get_Handle() const { return m_handle; }
	};

	typedef Handle<HANDLE, &::CloseHandle> RAIIHANDLE;
	typedef Handle<HMODULE, &::FreeLibrary> RAIIHMODULE;
	typedef Handle<HDESK, &::CloseDesktop> RAIIHDESKTOP;	
	typedef Handle<SC_HANDLE, &::CloseServiceHandle> RAIISC_HANDLE;
}

#endif