#include "stdafx.h"
#include "ProcessUtils.h"
#include "Lmcons.h"
#include "Shellapi.h"
#include <lm.h>
#include "Handle_Wrapper.h"
#include "Userenv.h"

bool RemoteDesktop::IsElevated() {
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
	return fRet == TRUE;
}

bool RemoteDesktop::IsUserAdmin(){
	HANDLE hTok = nullptr;
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hTok))
	{
		DEBUG_MSG("Cannot open thread token, trying process token %", GetLastError());
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok))
		{
			DEBUG_MSG("Cannot open process token, quitting %", GetLastError());
			return false;
		}
	}
	auto threadtok(RAIIHANDLE(hTok));//make sure its destoryed properly

	auto found = IsUserAdmin(hTok);
	if (!found){//search domain groups as well

		wchar_t user_name[256];
		DWORD size = sizeof(user_name);
		GetUserNameW(user_name, &size);
		found = IsUserAdmin(user_name);
	}
	return found;
}
bool RemoteDesktop::IsUserAdmin(std::wstring username){
	DWORD rc;
	USER_INFO_1 *info;
	auto splits = split(username, L'\\');
	if (splits.size() == 2) rc = NetUserGetInfo(splits[0].c_str(), splits[1].c_str(), 1, (LPBYTE *)&info);
	else rc = NetUserGetInfo(NULL, username.c_str(), 1, (LPBYTE *)&info);
	if (rc != NERR_Success)
		return false;

	auto found = info->usri1_priv == USER_PRIV_ADMIN;
	NetApiBufferFree(info);
	return found;
}
bool RemoteDesktop::IsUserAdmin(HANDLE hTok){

	bool found;
	DWORD  l(0);
	
	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;

	if (!GetTokenInformation(hTok, TokenGroups, NULL, 0, &l))
	{
		auto dwResult = GetLastError();
		if (dwResult != ERROR_INSUFFICIENT_BUFFER) {
			printf("GetTokenInformation Error %u\n", dwResult);
			return false;
		}
	}
	std::vector<char> buffer;
	buffer.reserve(l);
	if (!GetTokenInformation(hTok, TokenGroups, buffer.data(), l, &l))
	{
		DEBUG_MSG("Cannot get group list from token [%].", GetLastError());
		return false;
	}
	PSID pAdminSid = nullptr;
	// here, we cobble up a SID for the Administrators group, to compare to.
	if (!AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid))
	{
		DEBUG_MSG("Cannot create SID for Administrators  [%].", GetLastError());
		return false;
	}
	PTOKEN_GROUPS groupList = (PTOKEN_GROUPS)buffer.data();
	// now, loop through groups in token and compare
	found = 0;
	for (auto i = 0; i < groupList->GroupCount; ++i)
	{
		if (EqualSid(pAdminSid, groupList->Groups[i].Sid))
		{
			found = true;
			break;
		}
	}
	if (pAdminSid) FreeSid(pAdminSid);
	return found;
}
bool RemoteDesktop::TryToElevate(LPWSTR* argv, int argc) {
	if (IsElevated()) return false;//allready elevated
	if (!IsUserAdmin()) return false;// cannot elevate process anyway
	//we have the power.. TRY TO ELVATE!!!

	// Launch itself as admin
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	std::wstring args;
	for (auto i = 1; i < argc; i++) {//ignore the first arg, its the app name
		args += L" ";
		args += argv[i];
	}
	wchar_t szPath[MAX_PATH];
	std::wstring tmp(argv[0]);
	tmp = L"\"" + tmp + L"\"";
	
	wcsncpy_s(szPath, tmp.c_str(), tmp.size() + 1);

	sei.lpVerb = L"runas";
	sei.lpFile = szPath;
	sei.lpParameters = args.c_str();
	sei.hwnd = NULL;
	sei.nShow = SW_NORMAL;
	ShellExecuteEx(&sei);
	return true;//
}

std::shared_ptr<PROCESS_INFORMATION> RemoteDesktop::LaunchProcess(wchar_t* commandline, HANDLE token){
	auto ProcessInfo = std::shared_ptr<PROCESS_INFORMATION>(new PROCESS_INFORMATION(), [=](PROCESS_INFORMATION* p){
		CloseHandle(p->hThread);
		CloseHandle(p->hProcess);
		delete p;
	});
	STARTUPINFO StartUPInfo;
	memset(&StartUPInfo, 0, sizeof(STARTUPINFO));
	memset(ProcessInfo.get(), 0, sizeof(PROCESS_INFORMATION));

	StartUPInfo.lpDesktop = L"Winsta0\\Winlogon";
	StartUPInfo.cb = sizeof(STARTUPINFO);
	PVOID	lpEnvironment = NULL;

	if (CreateEnvironmentBlock(&lpEnvironment, token, FALSE) == FALSE) lpEnvironment = NULL;
	if (!CreateProcessAsUserW(token, NULL, (wchar_t*)commandline, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | DETACHED_PROCESS, lpEnvironment, NULL, &StartUPInfo, ProcessInfo.get())){
		DEBUG_MSG("Failed to created process %", GetLastError());
	}
	if (lpEnvironment) DestroyEnvironmentBlock(lpEnvironment);
	return ProcessInfo;
}

bool RemoteDesktop::LaunchProcess(wchar_t* commandline, const wchar_t* user, const wchar_t* domain, const wchar_t* pass, HANDLE token){
	auto ProcessInfo = std::shared_ptr<PROCESS_INFORMATION>(new PROCESS_INFORMATION(), [=](PROCESS_INFORMATION* p){
		CloseHandle(p->hThread);
		CloseHandle(p->hProcess);
		delete p;
	});
	STARTUPINFO StartUPInfo = { 0 };
	memset(ProcessInfo.get(), 0, sizeof(PROCESS_INFORMATION));
	StartUPInfo.cb = sizeof(STARTUPINFO);

	PVOID	lpEnvironment = NULL;
	if (CreateEnvironmentBlock(&lpEnvironment, token, FALSE) == FALSE) lpEnvironment = NULL;
	bool success = true;
	if (!CreateProcessWithLogonW(user, domain, pass, 0, NULL, commandline, CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, lpEnvironment, NULL, &StartUPInfo, ProcessInfo.get())){
		DEBUG_MSG("Failed to created process %", GetLastError());
		success = false;
	}
	if (lpEnvironment) DestroyEnvironmentBlock(lpEnvironment);

	return success;
}
bool RemoteDesktop::TryToElevate(std::wstring user, std::wstring pass){
	HANDLE token = NULL;
	if (user.size() < 2 || pass.size() < 2) return false;
	auto splits = split(user, L'\\');
	bool authenticated = false;
	if (splits.size() == 2){
		authenticated = LogonUser(splits[1].c_str(), splits[0].c_str(), pass.c_str(), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token) == TRUE;
	}
	else {
		authenticated = LogonUser(user.c_str(), NULL, pass.c_str(), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token) == TRUE;
	}
	if (authenticated){

		auto isadmin = IsUserAdmin(token);
		if (!isadmin && splits.size() == 2){//check domain controller for admin group
			isadmin = IsUserAdmin(user);
		}
		if (isadmin && !IsElevated()) {
			wchar_t szPath[MAX_PATH];
			GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
			std::wstring tmp(szPath);
			tmp = L"\"" + tmp + L"\"";
			wcsncpy_s(szPath, tmp.c_str(), tmp.size()+1);
			wchar_t cmndargs[] = L" -delayed_run";
			wcscat_s(szPath, cmndargs);

			if (splits.size() == 2){
				return LaunchProcess(szPath, splits[1].c_str(), splits[0].c_str(), pass.c_str(), token);
			}
			else {
				return LaunchProcess(szPath, user.c_str(), NULL, pass.c_str(), token);
			}
		}
	}
	if (token != nullptr) CloseHandle(token);
	return false;
}