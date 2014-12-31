#ifndef RD_CONFIG_123_H
#define RD_CONFIG_123_H

#define SERVICE_START_TYPE       SERVICE_DEMAND_START

wchar_t* Service_Name();
wchar_t* Service_Display_Name();
int DefaultPort();
wchar_t* DefaultGateway();
wchar_t* DefaultProxyGetSessionURL();
wchar_t* DisclaimerMessage();
wchar_t* Unique_ID();

#endif