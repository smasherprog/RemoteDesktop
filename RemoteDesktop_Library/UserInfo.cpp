#include "stdafx.h"
#include "UserInfo.h"
#include"ProcessUtils.h"
#include "NetworkSetup.h"
#include <lm.h>
#include <iostream>
#include "Wtsapi32.h"
#include "Sddl.h"

RemoteDesktop::User_Info_Header RemoteDesktop::GetUserInfo(){
	DEBUG_MSG("GetUserInfo");
	static RemoteDesktop::User_Info_Header CachedUserInfo = { 0 };
	if (wcsnlen_s(CachedUserInfo.name, ARRAYSIZE(CachedUserInfo.name)) > 2) return CachedUserInfo;//return a copy
	DWORD len = UNAMELEN; 
	GetComputerNameExW(ComputerNameDnsDomain, CachedUserInfo.domain, &len);
	len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameExW(ComputerNameDnsFullyQualified, CachedUserInfo.computername, &len);

	DWORD username_len = UNAMELEN;
	auto user = s2ws(get_ActiveUser());
	wcsncpy_s(CachedUserInfo.name, user.c_str(), user.size()+1);
	USER_INFO_2 *info;
	if (wcsnlen_s(CachedUserInfo.domain, ARRAYSIZE(CachedUserInfo.domain)) > 1){
		DWORD rc = NetUserGetInfo(CachedUserInfo.domain, CachedUserInfo.name, 2, (LPBYTE *)&info);
		if (rc == NERR_Success){
			wcsncpy_s(CachedUserInfo.full_name, info->usri2_full_name, ARRAYSIZE(CachedUserInfo.full_name));
			NetApiBufferFree(info);
		}
		else {
			wcsncpy_s(CachedUserInfo.full_name, CachedUserInfo.name, ARRAYSIZE(CachedUserInfo.full_name));
		
		}
	}
	else {
		wcsncpy_s(CachedUserInfo.full_name, CachedUserInfo.name, ARRAYSIZE(CachedUserInfo.full_name));
	}	
	if (wcsnlen_s(CachedUserInfo.domain, ARRAYSIZE(CachedUserInfo.domain)) <= 1){//use the PC's name in the case no domain information is found. This will allow for concat of things like Domain\\User
		wcsncpy_s(CachedUserInfo.domain, CachedUserInfo.computername, wcsnlen_s(CachedUserInfo.computername, ARRAYSIZE(CachedUserInfo.computername)));
	}
	CachedUserInfo.isadmin = IsUserAdmin();
	auto ip = s2ws(GetIP());
	wcsncpy_s(CachedUserInfo.ip_addr, ip.c_str(), ip.size()+1); 
	return CachedUserInfo;
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
bool RemoteDesktop::GetUserName_From_SID(std::wstring SID, std::wstring& username){
	//Future work. . . .. 
	//LookupAccountSidW()
	return false;//
}
bool RemoteDesktop::GetSid_From_Username(std::wstring username, std::wstring& SID){
	DWORD sidsize(0), domainsize(0);
	SID_NAME_USE    snu;
	//get size of needed buffers. I expect the function to fail returning zero, this is normal. As long as the error  ==  ERROR_INSUFFICIENT_BUFFER
	if (LookupAccountNameW(NULL, username.c_str(), NULL, &sidsize, NULL, &domainsize, &snu) == 0){
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER){
			SID = L"";
			return false;
		}
	}

	std::vector<char> sidbuffer;
	sidbuffer.reserve(sidsize);//returned size is in bytes
	std::vector<char> domainbuffer;
	domainbuffer.reserve(domainsize * 2);//returned size is in TCHARS!!!

	if (LookupAccountNameW(NULL, username.c_str(), sidbuffer.data(), &sidsize, (wchar_t*)domainbuffer.data(), &domainsize, &snu) == 0){
		SID = L"";
		auto ret = GetLastError();
		return false;
	}
	wchar_t* outbuffer = nullptr;

	if (ConvertSidToStringSidW(sidbuffer.data(), &outbuffer) == TRUE){
		SID = outbuffer;
		LocalFree(outbuffer);
		return true;
	}
	SID = L"";
	return false;
}