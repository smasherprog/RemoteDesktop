#include "stdafx.h"
#include "ServiceHelpers.h"
#include <tlhelp32.h>
#include "Wtsapi32.h"

unsigned int GetProcessesByName(wchar_t* process){
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	auto pid = 0;
	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_wcsicmp(entry.szExeFile, process) == 0)
			{
				pid = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(snapshot);
	return pid;
}

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

	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
//	DWORD Id = GetProcessesByName(L"winlogon.exe");
	DWORD Id = Find_winlogon(sessionid);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id);
	if (hProcess)
	{
		if (OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, &hTokenThis))
		{
			bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
			CloseHandle(hTokenThis);
		}
		CloseHandle(hProcess);
	}
	return bResult == 1;
}

