#include "stdafx.h"
#include "UserInfo.h"
#include"ProcessUtils.h"
#include "NetworkSetup.h"
#include <lm.h>
#include <iostream>

RemoteDesktop::User_Info_Header RemoteDesktop::GetUserInfo(){


	User_Info_Header h = { 0 };
	DWORD len = UNAMELEN;
	GetComputerNameEx(ComputerNameDnsDomain, h.domain, &len);
	std::wcout << h.domain << std::endl;
	len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameEx(ComputerNameDnsFullyQualified, h.computername, &len);
	std::wcout << h.computername << std::endl;
	DWORD username_len = UNAMELEN + 1;
	GetUserName(h.name, &username_len);
	h.name[username_len] = 0;
	std::wcout << h.name << std::endl;
	USER_INFO_2 *info;
	if (wcsnlen_s(h.domain, ARRAYSIZE(h.domain)) > 1){
		DWORD rc = NetUserGetInfo(h.domain, h.name, 2, (LPBYTE *)&info);
		if (rc == NERR_Success){
			wcsncpy_s(h.full_name, info->usri2_full_name, ARRAYSIZE(h.full_name));	
			std::wcout <<L"1"<< h.full_name << std::endl;
			NetApiBufferFree(info);
		}
		else {
			std::wcout << L"2" << h.full_name << std::endl;
			wcsncpy_s(h.full_name, h.name, ARRAYSIZE(h.full_name));
		}
	}
	else {
		std::wcout << L"3" << h.full_name << std::endl;
		wcsncpy_s(h.full_name, h.name, ARRAYSIZE(h.full_name));
	}
	h.isadmin = IsUserAdmin();
	auto ip = s2ws(GetIP()); 
	std::wcout << ip << std::endl;
	wcsncpy_s(h.ip_addr, ip.c_str(), ARRAYSIZE(h.ip_addr));
	std::wcout << L"Done"<< std::endl;
	return h;
}