#ifndef RD_CONFIG_123_H
#define RD_CONFIG_123_H

#define SERVICE_START_TYPE       SERVICE_DEMAND_START
#define RAT_TOOLCONFIG_FILE "rat_toolConfig92.txt";//config name

namespace RemoteDesktop{

	class Global_Settings{
	public:
		Global_Settings();
		wchar_t Service_Name[128];
		wchar_t Service_Display_Name[128];
		wchar_t DefaultPort[8];
		wchar_t DefaultGateway[128];
		wchar_t DefaultProxyGetSessionURL[256];
		wchar_t DisclaimerMessage[512];
		wchar_t Unique_ID[18];
		wchar_t Last_UserConnectName[128];
		void FlushToDisk();
	};	
	namespace INTERNAL{
		extern Global_Settings _Global_Settings;
	}
}
wchar_t* Service_Name();
wchar_t* Service_Display_Name();
wchar_t* DefaultPort();
wchar_t* DefaultGateway();
wchar_t* DefaultProxyGetSessionURL();
wchar_t* DisclaimerMessage();
wchar_t* Unique_ID();
wchar_t* GetLast_UserConnectName();
void SetLast_UserConnectName(std::wstring name);
#endif