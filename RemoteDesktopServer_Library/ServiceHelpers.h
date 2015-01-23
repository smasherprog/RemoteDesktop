#ifndef SERVICEHELPER123_H
#define SERVICEHELPER123_H
#include <memory>

bool GetWinlogonHandle(LPHANDLE  lphUserToken, DWORD sessionid);

std::shared_ptr<PROCESS_INFORMATION> LaunchProcess(wchar_t* cmd, DWORD sessionid);


#endif