#ifndef RD_CONFIG_123_H
#define RD_CONFIG_123_H

// Internal name of the service
#define SERVICE_NAME             L"RemoteDesktop_svc"
// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"RemoteDesktop Service"
// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START
#define DEFAULTPORT 443
#define DEFAULTPROXY L"127.0.0.1"//this should be the ip address or hostname of the proxy server to initiate a reverse connection with..


#endif