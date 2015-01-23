#ifndef PROCESSUTILS123_H
#define PROCESSUTILS123_H
#include <memory>

namespace RemoteDesktop{
	bool IsElevated();
	bool IsUserAdmin();
	bool IsUserAdmin(std::wstring username);
	bool IsUserAdmin(HANDLE hTok);

	//command line args so the process can be restarted
	//returns true if the process successfully started elevated
	bool TryToElevate(wchar_t** argv, int argc);

	std::shared_ptr<PROCESS_INFORMATION> LaunchProcess(wchar_t* commandline, HANDLE token);
	bool LaunchProcess(wchar_t* commandline, const wchar_t* user, const wchar_t* domain, const wchar_t* pass, HANDLE token);


}


#endif