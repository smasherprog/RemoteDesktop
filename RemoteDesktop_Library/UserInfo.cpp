#include "stdafx.h"
#include "UserInfo.h"
#include"ProcessUtils.h"
#include <lm.h>

RemoteDesktop::User_Info_Header RemoteDesktop::GetUserInfo(){


	User_Info_Header h;

	DWORD len = UNAMELEN;
	GetComputerNameEx(ComputerNameDnsDomain, h.domain, &len);

	len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameEx(ComputerNameDnsFullyQualified, h.computername, &len);

	DWORD username_len = UNAMELEN + 1;
	GetUserName(h.name, &username_len);
	h.name[username_len] = 0;


	wchar_t user_name[256];
	USER_INFO_2 *info;
	DWORD size = sizeof(user_name);
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
	return h;
}