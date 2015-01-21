#include "stdafx.h"
#include "ServiceHelpers.h"
#include <tlhelp32.h>
#include "Wtsapi32.h"
#include "Userenv.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"

DWORD
Find_winlogon(DWORD SessionId)
{

	PWTS_PROCESS_INFOA pProcessInfo = NULL;
	DWORD         ProcessCount = 0;
	//  char         szUserName[255];
	DWORD         Id = -1;

	typedef BOOL(WINAPI *pfnWTSEnumerateProcesses)(HANDLE, DWORD, DWORD, PWTS_PROCESS_INFOA*, DWORD*);
	typedef VOID(WINAPI *pfnWTSFreeMemory)(PVOID);

	DynamicFn<pfnWTSEnumerateProcesses> pWTSEnumerateProcesses("wtsapi32", "WTSEnumerateProcessesA");
	DynamicFn<pfnWTSFreeMemory> pWTSFreeMemory("wtsapi32", "WTSFreeMemory");

	if (pWTSEnumerateProcesses.isValid() && pWTSFreeMemory.isValid())
	{
		if ((*pWTSEnumerateProcesses)(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount))
		{
			// dump each process description
			for (DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++)
			{

				if (_stricmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0)
				{
					if (SessionId == pProcessInfo[CurrentProcess].SessionId)
					{
						Id = pProcessInfo[CurrentProcess].ProcessId;
						break;
					}
				}
			}

			(*pWTSFreeMemory)(pProcessInfo);
		}
	}

	return Id;
}

bool GetWinlogonHandle(LPHANDLE  lphUserToken, DWORD sessionid)
{
	BOOL   bResult = FALSE;

	HANDLE hTokenThis = NULL;
	DWORD Id = Find_winlogon(sessionid);
	auto hProcess(RAIIHANDLE(OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id)));
	if (hProcess.get())
	{
		if (OpenProcessToken(hProcess.get(), TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &hTokenThis))
		{
			bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
			CloseHandle(hTokenThis);
		}
	}
	return bResult == 1;
}

