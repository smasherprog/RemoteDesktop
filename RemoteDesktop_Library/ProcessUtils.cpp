#include "stdafx.h"
#include "ProcessUtils.h"
#include "Lmcons.h"
#include "Shellapi.h"
#include <lm.h>
#include "Handle_Wrapper.h"

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
		DEBUG_MSG("Cannot open thread token, trying process token [%].", GetLastError());
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok))
		{
			DEBUG_MSG("Cannot open process token, quitting [%].", GetLastError());
			return false;
		}
	}
	auto threadtok(RAIIHANDLE(hTok));//make sure its destoryed properly

	auto found = IsUserAdmin(hTok);
	if (!found){//search domain groups as well
		DWORD rc;
		wchar_t user_name[256];
		USER_INFO_1 *info;
		DWORD size = sizeof(user_name);

		GetUserNameW(user_name, &size);

		rc = NetUserGetInfo(NULL, user_name, 1, (LPBYTE *)&info);
		if (rc != NERR_Success)
			return false;

		found = info->usri1_priv == USER_PRIV_ADMIN;
		NetApiBufferFree(info);
	}
	return found;
}

bool RemoteDesktop::IsUserAdmin(HANDLE hTok){

	bool found;
	DWORD i, l;
	PSID pAdminSid = nullptr;
	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;

	BYTE rawGroupList[4096];
	TOKEN_GROUPS& groupList = *((TOKEN_GROUPS *)rawGroupList);

	// normally, I should get the size of the group list first, but ...
	l = sizeof(rawGroupList);
	if (!GetTokenInformation(hTok, TokenGroups, &groupList, l, &l))
	{
		DEBUG_MSG("Cannot get group list from token [%].", GetLastError());
		return false;
	}

	// here, we cobble up a SID for the Administrators group, to compare to.
	if (!AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid))
	{
		DEBUG_MSG("Cannot create SID for Administrators  [%].", GetLastError());
		return false;
	}

	// now, loop through groups in token and compare
	found = 0;
	for (i = 0; i < groupList.GroupCount; ++i)
	{
		if (EqualSid(pAdminSid, groupList.Groups[i].Sid))
		{
			found = true;
			break;
		}
	}

	FreeSid(pAdminSid);
	return found;
}
bool RemoteDesktop::TryToElevate(LPWSTR* argv, int argc){
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
	sei.lpVerb = L"runas";
	sei.lpFile = argv[0];//first arg is the full path to the exe
	sei.lpParameters = args.c_str();
	sei.hwnd = NULL;
	sei.nShow = SW_NORMAL;
	ShellExecuteEx(&sei);
	return true;//
}
