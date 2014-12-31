#ifndef SERVICEHELPER123_H
#define SERVICEHELPER123_H
#include <memory>

unsigned int GetProcessesByName(wchar_t* process);

bool GetWinlogonHandle(LPHANDLE  lphUserToken, DWORD sessionid);

std::shared_ptr<PROCESS_INFORMATION> LaunchProcess(wchar_t* cmd, DWORD sessionid);

BOOL CreateRemoteSessionProcess(
	IN DWORD        dwSessionId,
	IN BOOL         bUseDefaultToken,
	IN HANDLE       hToken,
	IN LPCWSTR      lpApplicationName,
	IN LPCWSTR       lpCommandLine,
	IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
	IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
	IN BOOL bInheritHandles,
	IN DWORD dwCreationFlags,
	IN LPVOID lpEnvironment,
	IN LPCWSTR lpCurrentDirectory,
	IN LPSTARTUPINFO A_lpStartupInfo,
	OUT LPPROCESS_INFORMATION lpProcessInformation);

#endif