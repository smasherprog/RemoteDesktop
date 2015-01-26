#ifndef USERNAMEINFO123_H
#define USERNAMEINFO123_H
#include "CommonNetwork.h"

namespace RemoteDesktop{
	User_Info_Header GetUserInfo();
	std::string get_ActiveUser();
	bool GetUserName_From_SID(std::wstring SID, std::wstring& username);
	bool GetSid_From_Username(std::wstring username, std::wstring& SID);
}

#endif