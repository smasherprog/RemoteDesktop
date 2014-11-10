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


#define MAXSTRLENGTH    255
BOOL Char2Wchar(WCHAR* pDest, char* pSrc, int nDestStrLen)
{
	int nSrcStrLen = 0;
	int nOutputBuffLen = 0;
	int retcode = 0;

	if (pDest == NULL || pSrc == NULL)
	{
		return FALSE;
	}

	nSrcStrLen = strlen(pSrc);
	if (nSrcStrLen == 0)
	{
		return FALSE;
	}

	nDestStrLen = nSrcStrLen;

	if (nDestStrLen > MAXSTRLENGTH - 1)
	{
		return FALSE;
	}
	memset(pDest, 0, sizeof(TCHAR)*nDestStrLen);
	nOutputBuffLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pSrc, nSrcStrLen, pDest, nDestStrLen);

	if (nOutputBuffLen == 0)
	{
		retcode = GetLastError();
		return FALSE;
	}

	pDest[nOutputBuffLen] = '\0';
	return TRUE;
}
typedef struct _CPAU_PARAM{
	DWORD   cbSize;
	DWORD   dwProcessId;
	BOOL    bUseDefaultToken;
	HANDLE  hToken;
	LPWSTR  lpApplicationName;
	LPWSTR  lpCommandLine;
	SECURITY_ATTRIBUTES     ProcessAttributes;
	SECURITY_ATTRIBUTES ThreadAttributes;
	BOOL bInheritHandles;
	DWORD dwCreationFlags;
	LPVOID lpEnvironment;
	LPWSTR lpCurrentDirectory;
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION     ProcessInformation;

}CPAU_PARAM;

typedef struct _CPAU_RET_PARAM{
	DWORD   cbSize;
	BOOL    bRetValue;
	DWORD   dwLastErr;
	PROCESS_INFORMATION     ProcInfo;

}CPAU_RET_PARAM;

typedef BOOLEAN(WINAPI* pWinStationQueryInformationW)(
	IN   HANDLE hServer,
	IN   ULONG LogonId,
	IN   DWORD /*WINSTATIONINFOCLASS*/ WinStationInformationClass,
	OUT  PVOID pWinStationInformation,
	IN   ULONG WinStationInformationLength,
	OUT  PULONG pReturnLength
	);
DWORD MarshallString(LPCWSTR    pszText, LPVOID, DWORD  dwMaxSize, LPBYTE*
	ppNextBuf, DWORD* pdwUsedBytes)
{
	DWORD   dwOffset = *pdwUsedBytes;
	if (!pszText)
		return 0;
	DWORD   dwLen = (wcslen(pszText) + 1)*sizeof(WCHAR);
	if (*pdwUsedBytes + dwLen> dwMaxSize)
		return 0;
	memmove(*ppNextBuf, pszText, dwLen);
	*pdwUsedBytes += dwLen;
	*ppNextBuf += dwLen;
	return dwOffset;

}
BOOL CreateRemoteSessionProcess(
	IN DWORD        dwSessionId,
	IN BOOL         bUseDefaultToken,
	IN HANDLE       hToken,
	IN LPCWSTR      lpApplicationName,
	IN LPCWSTR       lpCommandLine,
	IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
	IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
	IN BOOL bInheritHandles,
	IN DWORD dwCreationFlags,
	IN LPVOID lpEnvironment,
	IN LPCWSTR lpCurrentDirectory,
	IN LPSTARTUPINFO A_lpStartupInfo,
	OUT LPPROCESS_INFORMATION lpProcessInformation)
{

	STARTUPINFOW StartupInfo;
	ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
	StartupInfo.wShowWindow = SW_SHOW;
	StartupInfo.lpDesktop = L"Winsta0\\Winlogon";
	StartupInfo.cb = sizeof(STARTUPINFOW);

	WCHAR           szWinStaPath[MAX_PATH];
	BOOL            bGetNPName = FALSE;
	WCHAR           szNamedPipeName[MAX_PATH] = L"";
	DWORD           dwNameLen;
	HINSTANCE       hInstWinSta;
	HANDLE          hNamedPipe;
	LPVOID          pData = NULL;
	BOOL            bRet = FALSE;
	DWORD           cbReadBytes, cbWriteBytes;
	DWORD           dwEnvLen = 0;
	union{
		CPAU_PARAM      cpauData;
		BYTE            bDump[0x2000];
	};
	CPAU_RET_PARAM  cpauRetData;
	DWORD                   dwUsedBytes = sizeof(cpauData);
	LPBYTE                  pBuffer = (LPBYTE)(&cpauData + 1);
	GetSystemDirectoryW(szWinStaPath, MAX_PATH);
	lstrcatW(szWinStaPath, L"\\winsta.dll");
	hInstWinSta = LoadLibraryW(szWinStaPath);

	if (hInstWinSta)
	{
		pWinStationQueryInformationW pfWinStationQueryInformationW = (pWinStationQueryInformationW)GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
		if (pfWinStationQueryInformationW)
		{
			bGetNPName = pfWinStationQueryInformationW(0, dwSessionId, 0x21, szNamedPipeName, sizeof(szNamedPipeName), &dwNameLen);
		}
		FreeLibrary(hInstWinSta);
	}
	if (!bGetNPName || szNamedPipeName[0] == '\0')
	{
		swprintf(szNamedPipeName, 260, L"\\\\.\\Pipe\\TerminalServer\\SystemExecSrvr\\%d", dwSessionId);
	}

	do{
		hNamedPipe = CreateFileW(szNamedPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
		if (hNamedPipe == INVALID_HANDLE_VALUE)
		{
			if (GetLastError() == ERROR_PIPE_BUSY)
			{
				if (!WaitNamedPipeW(szNamedPipeName, 30000))
					return FALSE;
			}
			else
			{
				return FALSE;
			}
		}
	} while (hNamedPipe == INVALID_HANDLE_VALUE);

	memset(&cpauData, 0, sizeof(cpauData));
	cpauData.bInheritHandles = bInheritHandles;
	cpauData.bUseDefaultToken = bUseDefaultToken;
	cpauData.dwCreationFlags = dwCreationFlags;
	cpauData.dwProcessId = GetCurrentProcessId();
	cpauData.hToken = hToken;
	cpauData.lpApplicationName = (LPWSTR)MarshallString(lpApplicationName, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.lpCommandLine = (LPWSTR)MarshallString(lpCommandLine, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.StartupInfo = StartupInfo;
	cpauData.StartupInfo.lpDesktop = (LPWSTR)MarshallString(cpauData.StartupInfo.lpDesktop, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);
	cpauData.StartupInfo.lpTitle = (LPWSTR)MarshallString(cpauData.StartupInfo.lpTitle, &cpauData, sizeof(bDump), &pBuffer, &dwUsedBytes);


	cpauData.lpEnvironment = NULL;
	cpauData.cbSize = dwUsedBytes;

	HANDLE hProcess = NULL;
	if (WriteFile(hNamedPipe, &cpauData, cpauData.cbSize, &cbWriteBytes, NULL))
	{
		Sleep(250);
		if (ReadFile(hNamedPipe, &cpauRetData, sizeof(cpauRetData), &cbReadBytes, NULL))
		{
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cpauRetData.ProcInfo.dwProcessId);
			bRet = cpauRetData.bRetValue;
			if (bRet)
			{
				*lpProcessInformation = cpauRetData.ProcInfo;
			}
			else
				SetLastError(cpauRetData.dwLastErr);
		}
	}
	else
		bRet = FALSE;
	// function sometimes fail, the use processid to get hprocess... bug MS
	if (lpProcessInformation->hProcess == 0) lpProcessInformation->hProcess = hProcess;
	//this should never happen, looping connections
	if (lpProcessInformation->hProcess == 0)
		Sleep(5000);
	CloseHandle(hNamedPipe);
	return bRet;

}