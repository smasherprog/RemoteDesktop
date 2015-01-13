#include "stdafx.h"
#include "Gateway.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\WinHttpClient.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"
#include "Config.h"

bool RemoteDesktop::GetGatewayID_and_Key(int& id, std::wstring& aeskey, std::wstring gatewayurl){
	char comp[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameA(comp, &len);
	std::string computername(comp);

	std::string username = RemoteDesktop::DesktopMonitor::get_ActiveUser();

	auto mac = GetMAC();
	std::string uniqueid = ws2s(Unique_ID());

	std::string adddata = "computername=" + computername + "&username=" + username + "&mac=" + mac + "&session=" + uniqueid;
	DEBUG_MSG("Getting ID: % %", ws2s(gatewayurl), adddata);
	WinHttpClient cl(gatewayurl);
	cl.SetAdditionalDataToSend((BYTE*)adddata.c_str(), adddata.size());

	// Set request headers.
	wchar_t szSize[50] = L"";
	swprintf_s(szSize, L"%d", adddata.size());
	std::wstring headers = L"Content-Length: ";
	headers += szSize;
	headers += L"\r\nContent-Type: application/x-www-form-urlencoded\r\n";

	cl.SetAdditionalRequestHeaders(headers);
	cl.SendHttpRequest(L"POST");
	id = -1;
	auto httpResponseContent = cl.GetResponseContent();
	if (httpResponseContent.size() > 0){
		auto splits = split(httpResponseContent, L'\n');
		if (splits.size() == 2){
			id = std::stoi(splits[0]);
			aeskey = splits[1];
		}
	}
	return id != -1;

}
