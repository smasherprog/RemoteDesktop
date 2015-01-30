#include "stdafx.h"
#include "Desktop_Background.h"
#include "Desktop_Monitor.h"
#include "UserInfo.h"
#include <shlobj.h>  // IActiveDesktop
#include <thread>


RemoteDesktop::DesktopBackground::DesktopBackground(){
	ZEROMEMORY(_Wallpaper_Style);
	ZEROMEMORY(_Wallpaper_Path);
}
RemoteDesktop::DesktopBackground::~DesktopBackground(){
	Restore();
}

void RemoteDesktop::DesktopBackground::Restore(){
	_RestoreWallpaper();
	if (_HidingWallpaper)_RestoreWallpaperStyle();
	_HidingWallpaper = false;
}
bool SetWallpaper(std::wstring path){
	DEBUG_MSG("SetWallpaper %", ws2s(path));

	HKEY hkLocal;
	if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hkLocal) == ERROR_SUCCESS){
		DWORD bytes = path.size() * 2;
		RegSetValueExW(hkLocal,
			L"Wallpaper",
			0,
			REG_SZ,
			(BYTE*)path.c_str(),
			bytes + 2);
		RegCloseKey(hkLocal);
	}


	return true;
}
void RemoteDesktop::DesktopBackground::_SaveWallpaperStyle(){
	HKEY hKey;

	if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD sz = sizeof(_Wallpaper_Style);
		::RegQueryValueExW(hKey, L"WallpaperStyle", 0, 0, (unsigned char*)_Wallpaper_Style, &sz);
		::RegCloseKey(hKey);
	}
}
void RemoteDesktop::DesktopBackground::_RestoreWallpaperStyle(){
	HKEY hKey;

	if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		DWORD sz = wcsnlen_s(_Wallpaper_Style, ARRAYSIZE(_Wallpaper_Style));
		::RegSetValueExW(hKey, L"WallpaperStyle", 0, REG_SZ, (unsigned char*)_Wallpaper_Style, sz);
		::RegCloseKey(hKey);
	}
}
bool RemoteDesktop::DesktopBackground::_SaveWallpaper(){
	HKEY hkLocal;
	DWORD dw;
	auto userinfo = GetUserInfo();
	auto fqname = std::wstring(userinfo.domain) + L"\\" + std::wstring(userinfo.name);
	DEBUG_MSG("DesktopBackground() %", ws2s(fqname));
	if (!GetSid_From_Username(fqname, UserSID))	return false;//nothing can be done.. !!
	auto regkey = UserSID + L"\\Control Panel\\Desktop";
	if (RegOpenKeyExW(HKEY_USERS, regkey.c_str(), 0, KEY_READ, &hkLocal) != ERROR_SUCCESS) return false;
	DWORD bytes = ARRAYSIZE(_Wallpaper_Path);
	auto ret = RegQueryValueEx(hkLocal, L"Wallpaper", 0, NULL, (BYTE*)_Wallpaper_Path, &bytes);
	RegCloseKey(hkLocal);
	return ret == ERROR_SUCCESS;
}
void RemoteDesktop::DesktopBackground::_RestoreWallpaper(){
	if (_HidingWallpaper) SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, _Wallpaper_Path, SPIF_SENDCHANGE);
}

void RemoteDesktop::DesktopBackground::HideWallpaper(){
	//the desktop has to be the default, not screen saver or logon
	if (!_HidingWallpaper)
	{
		if (_SaveWallpaper()){
			_SaveWallpaperStyle();
			SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, L"", SPIF_SENDCHANGE);
			_HidingWallpaper = true;
		}

	}

}

