#include "stdafx.h"
#include "ServiceHelpers.h"
#include <tlhelp32.h>

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


bool GetWinlogonHandle(LPHANDLE  lphUserToken)
{
	BOOL   bResult = FALSE;

	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
	DWORD ID_session = 0;
	DWORD Id = GetProcessesByName(L"winlogon.exe");

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id);
	if (hProcess)
	{
		if (OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hTokenThis))
		{
			bResult = DuplicateTokenEx(hTokenThis, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
			CloseHandle(hTokenThis);
		}
		CloseHandle(hProcess);
	}
	return bResult == 1;
}
