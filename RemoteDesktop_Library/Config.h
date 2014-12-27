#ifndef RD_CONFIG_123_H
#define RD_CONFIG_123_H

// Internal name of the service
#define SERVICE_NAME             L"RemoteDesktop_svc"
// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"RemoteDesktop Service"
// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START
#define DEFAULTPORT 443
#define DEFAULTPROXY L"localhost"//this should be the ip address or hostname of the proxy server to initiate a reverse connection with..

#define DEFAULTPROXYWEB L"localhost:1466"
#define DEFAULTPROXYWEBAUTHPATH L"/Home/Authenticate"
#define DEFAULTSIGNALRHUBNAME L"ProxyHub"
#define DEFAULTURISCHEME L"http://"
#define DEFAULTGETIDPATH L"/Home/GetID"

#define DEFAULTPROXYGETSESSIONURL (std::wstring(DEFAULTURISCHEME) +  std::wstring(DEFAULTPROXYWEB) + std::wstring(DEFAULTGETIDPATH))//this should be the ip address or hostname of the proxy server to initiate a reverse connection with..


#endif