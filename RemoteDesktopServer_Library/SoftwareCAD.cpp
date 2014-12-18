#include "stdafx.h"
#include "SoftwareCAD.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"

bool RemoteDesktop::IsSASCadEnabled()
{
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies", 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) return false;
	if (RegOpenKeyEx(hkLocal,L"System",0, KEY_READ, &hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return false;
	}

	LONG pref = 0;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

	if (RegQueryValueEx(hkLocalKey,L"SoftwareSASGeneration",NULL,&type,(LPBYTE)&pref,&prefsize) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocalKey);
		RegCloseKey(hkLocal);
		return false;
	}
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
	if (pref != 0) return true;
	else return false;
}

void RemoteDesktop::Enable_SASCAD()
{
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies", 0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) return;
	if (RegOpenKeyEx(hkLocal,
		L"System",
		0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return;
	}
	LONG pref;
	pref = 1;
	RegSetValueEx(hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE)&pref, sizeof(pref));
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}
