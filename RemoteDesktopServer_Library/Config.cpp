#include "stdafx.h"
#include "Config.h"

wchar_t* Service_Name(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGSERVICE_NAME, b, 255);
	return b;
}
wchar_t* Service_Display_Name(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGSERVICE_DISPLAY_NAME, b, 255);
	return b;
}
int DefaultPort(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPORT, b, 255);
	return std::stoi(b);
}
wchar_t* DefaultGateway(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTGATEWAY, b, 255);
	return b;
}
wchar_t* DefaultProxyGetSessionURL(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDEFAULTPROXYGETSESSIONURL, b, 255);
	return b;
}
wchar_t* DisclaimerMessage(){
	static wchar_t b[512];
	LoadString(GetModuleHandle(NULL), IDS_STRINGDISCLAIMERMESSAGE, b, 255);
	return b;
}
wchar_t* Unique_ID(){
	static wchar_t b[256];
	LoadString(GetModuleHandle(NULL), IDS_STRINGUNIQUE_ID, b, 255);
	return b;
}
