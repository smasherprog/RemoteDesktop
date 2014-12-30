#ifndef RD_CONFIG_123_H
#define RD_CONFIG_123_H

#define SERVICE_START_TYPE       SERVICE_DEMAND_START

wchar_t* Service_Name();
wchar_t* Service_Display_Name();
int DefaultPort();
wchar_t* DefaultProxy();
wchar_t* DefaultProxyWeb();
wchar_t* DefaultProxyWebAuthPath();
wchar_t* DefaultSignalRHubName();
wchar_t* DefaultURIScheme();
wchar_t* DefaultGetIDPath();
wchar_t* DefaultProxyGetSessionURL();
wchar_t* DisclaimerMessage();

#endif