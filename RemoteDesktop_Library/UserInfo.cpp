#include "stdafx.h"
#include "UserInfo.h"
#include"ProcessUtils.h"
#include "NetworkSetup.h"
#include <lm.h>
#include <iostream>
#include "Wtsapi32.h"

RemoteDesktop::User_Info_Header RemoteDesktop::GetUserInfo(){


	User_Info_Header h = { 0 };
	DWORD len = UNAMELEN;
	GetComputerNameEx(ComputerNameDnsDomain, h.domain, &len);

	len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameEx(ComputerNameDnsFullyQualified, h.computername, &len);

	DWORD username_len = UNAMELEN + 1;
	auto user = s2ws(get_ActiveUser());
	wcsncpy_s(h.name, user.c_str(), username_len);

	USER_INFO_2 *info;
	if (wcsnlen_s(h.domain, ARRAYSIZE(h.domain)) > 1){
		DWORD rc = NetUserGetInfo(h.domain, h.name, 2, (LPBYTE *)&info);
		if (rc == NERR_Success){
			wcsncpy_s(h.full_name, info->usri2_full_name, ARRAYSIZE(h.full_name));
			NetApiBufferFree(info);
		}
		else {
			wcsncpy_s(h.full_name, h.name, ARRAYSIZE(h.full_name));
		}
	}
	else {
		wcsncpy_s(h.full_name, h.name, ARRAYSIZE(h.full_name));
	}
	h.isadmin = IsUserAdmin();
	auto ip = s2ws(GetIP());
	wcsncpy_s(h.ip_addr, ip.c_str(), ARRAYSIZE(h.ip_addr));
	return h;
}
std::string RemoteDesktop::get_ActiveUser(){
	static std::string CachedName;
	if (!CachedName.empty()) {
		if (find_substr(CachedName, std::string("system")) == -1) return CachedName;// dont call below code, it emits an anooying warning to the console when debugging
	}
	char* ptr = NULL;
	DWORD size = 0;
	if (WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTSGetActiveConsoleSessionId(), WTS_INFO_CLASS::WTSUserName, &ptr, &size)){
		CachedName = std::string(ptr);
		WTSFreeMemory(ptr);
	}
	if (!CachedName.empty()) {
		if (find_substr(CachedName, std::string("system")) == -1) return CachedName;// dont call below code, it emits an anooying warning to the console when debugging
	}
	//if the name is still the system account, try to check the regular logged in user
	DWORD username_len = UNAMELEN + 1;
	wchar_t user[UNAMELEN + 1];
	if (GetUserName(user, &username_len)){
		CachedName = ws2s(std::wstring(user));
	}

	return CachedName;
}