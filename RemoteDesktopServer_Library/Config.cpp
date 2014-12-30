#include "stdafx.h"
#include "Config.h"

wchar_t* Service_Name(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGSERVICE_NAME, b, 255);
	return b;
}
wchar_t* Service_Display_Name(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGSERVICE_DISPLAY_NAME, b, 255);
	return b;
}
int DefaultPort(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPORT, b, 255);
	return std::stoi(b);
}
wchar_t* DefaultProxy(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPROXY, b, 255);
	return b;
}
wchar_t* DefaultProxyWeb(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPROXYWEB, b, 255);
	return b;
}
wchar_t* DefaultProxyWebAuthPath(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPROXYWEBAUTHPATH, b, 255);
	return b;
}
wchar_t* DefaultSignalRHubName(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTSIGNALRHUBNAME, b, 255);
	return b;
}
wchar_t* DefaultURIScheme(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTURISCHEME, b, 255);
	return b;
}
wchar_t* DefaultGetIDPath(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTGETIDPATH, b, 255);
	return b;
}
wchar_t* DefaultProxyGetSessionURL(){
	static wchar_t b[255];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPROXYGETSESSIONURL, b, 255);
	return b;
}
wchar_t* DisclaimerMessage(){
	static wchar_t b[512];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDISCLAIMERMESSAGE, b, 255);
	
	return b;
}