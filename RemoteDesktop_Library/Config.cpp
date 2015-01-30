#include "stdafx.h"
#include "Config.h"
#include <random>

RemoteDesktop::Global_Settings RemoteDesktop::INTERNAL::_Global_Settings;

RemoteDesktop::Global_Settings::Global_Settings(){
	memset(this, 0, sizeof(this));
	wcsncpy_s(Service_Name, L"RAT_svc", ARRAYSIZE(Service_Name));
	wcsncpy_s(Service_Display_Name, L"Remote Access Tool", ARRAYSIZE(Service_Display_Name));
	wcsncpy_s(DefaultPort, L"443", ARRAYSIZE(DefaultPort));
	wcsncpy_s(DefaultGateway, L"localhost", ARRAYSIZE(DefaultGateway));
	wcsncpy_s(DefaultProxyGetSessionURL, L"http://localhost:3406/Support/GetID", ARRAYSIZE(DefaultProxyGetSessionURL));
	wcsncpy_s(DisclaimerMessage, L"Do you agree to allow a support technician to connection to your computer?", ARRAYSIZE(DisclaimerMessage));
	wcsncpy_s(Unique_ID, L"", ARRAYSIZE(Unique_ID));
	wcsncpy_s(Last_UserConnectName, L"", ARRAYSIZE(Last_UserConnectName));

	auto config = GetExePath() + "\\" + RAT_TOOLCONFIG_FILE;
	if (FileExists(config)){//file exists,read it in
		std::ifstream configfile(config.c_str(), std::ios::binary);
		configfile.read((char*)RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID, sizeof(RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID));
		configfile.read((char*)RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName, sizeof(RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName));
	}
}
void RemoteDesktop::Global_Settings::FlushToDisk(){
	auto config = GetExePath() + "\\" + RAT_TOOLCONFIG_FILE;
	std::ofstream configfile(config.c_str(), std::ios::binary | std::ios::trunc);//clear the file each flush
	configfile.write((char*)RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID, sizeof(RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID));
	configfile.write((char*)RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName, sizeof(RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName));
}
wchar_t* Service_Name(){
	return RemoteDesktop::INTERNAL::_Global_Settings.Service_Name;
}
wchar_t* Service_Display_Name(){
	return RemoteDesktop::INTERNAL::_Global_Settings.Service_Display_Name;
}
wchar_t* DefaultPort(){
	return RemoteDesktop::INTERNAL::_Global_Settings.DefaultPort;
}
wchar_t* DefaultGateway(){
	return RemoteDesktop::INTERNAL::_Global_Settings.DefaultGateway;
}
wchar_t* DefaultProxyGetSessionURL(){
	return RemoteDesktop::INTERNAL::_Global_Settings.DefaultProxyGetSessionURL;
}
wchar_t* DisclaimerMessage(){
	return RemoteDesktop::INTERNAL::_Global_Settings.DisclaimerMessage;
}

wchar_t* Unique_ID(){

	if (RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID[0] == 0 || RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID[0] == L'\n'){//dont have an id try to load or generate it
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<double> dist(MININT32, MAXINT32);
		int randint = (int)dist(mt);
		auto uniqueid = std::to_wstring(randint);
		//update file and settings
		wcsncpy_s(RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID, uniqueid.c_str(), uniqueid.size()+1);
		RemoteDesktop::INTERNAL::_Global_Settings.FlushToDisk();
	}
	return RemoteDesktop::INTERNAL::_Global_Settings.Unique_ID;
}
wchar_t* GetLast_UserConnectName(){
	return RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName;
}
void SetLast_UserConnectName(std::wstring name){
	wcsncpy_s(RemoteDesktop::INTERNAL::_Global_Settings.Last_UserConnectName, name.c_str(), name.size()+1);
	RemoteDesktop::INTERNAL::_Global_Settings.FlushToDisk();
}