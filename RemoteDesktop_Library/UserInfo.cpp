#include "stdafx.h"
#include "UserInfo.h"
#include "lm.h"
#include"ProcessUtils.h"

RemoteDesktop::User_Info_Header RemoteDesktop::GetUserInfo(){


	User_Info_Header h;
	DWORD username_len = UNAMELEN + 1;
	GetUserName(h.name, &username_len);
	h.name[username_len] = 0;

	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName(h.computername, &len);

	WKSTA_INFO_100 info = { 0 };
	if (NERR_Success == NetWkstaGetInfo(L"THIS-COMPUTER", 100, (LPBYTE*)&info)){
		wcsncpy_s(h.domain, info.wki100_langroup, ARRAYSIZE(h.domain));
		LPUSER_INFO_2 pBuf2 = NULL;
		NetUserGetInfo(h.domain, h.name, 2, (LPBYTE *)&pBuf2);
		wcsncpy_s(h.full_name,  pBuf2->usri2_full_name, ARRAYSIZE(h.full_name));
		h.priv = pBuf2->usri2_priv;
		if (pBuf2 != NULL) NetApiBufferFree(pBuf2);
	}
	else {

		h.priv = IsUserAdmin() ? USER_PRIV_ADMIN : USER_PRIV_USER;
		wcsncpy_s(h.full_name, h.name, ARRAYSIZE(h.full_name));
		wcsncpy_s(h.domain, L"No Domain", ARRAYSIZE(h.domain));
	}
	return h;
}