#include "stdafx.h"
#include "Gateway.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\WinHttpClient.h"
#include "..\RemoteDesktop_Library\Desktop_Monitor.h"
#include "..\RemoteDesktop_Library\NetworkSetup.h"
#include "Config.h"

int GetFileCreateTime()//used for randomness in session 
{
	wchar_t szPath[MAX_PATH];
	bool ret = false;
	if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) return 0;

	auto hFile(RAIIHANDLE(CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)));

	if (hFile.get() == INVALID_HANDLE_VALUE) return 0;

	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;


	// Retrieve the file times for the file.
	if (!GetFileTime(hFile.get(), &ftCreate, &ftAccess, &ftWrite))
		return 0;


	FileTimeToSystemTime(&ftCreate, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	return (int)stLocal.wMonth + (int)stLocal.wDay + (int)stLocal.wYear + (int)stLocal.wHour + (int)stLocal.wMinute + (int)stLocal.wSecond + (int)stLocal.wMilliseconds;//this is just some randomness to add into the mix

}
bool RemoteDesktop::GetGatewayID_and_Key(int& id, std::wstring& aeskey){
	char comp[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameA(comp, &len);
	std::string computername(comp);

	std::string username = RemoteDesktop::DesktopMonitor::get_ActiveUser();

	auto mac = GetMAC();
	std::wstring uniqueid(Unique_ID());
	if (uniqueid.size() <= 3){
		uniqueid = std::to_wstring(GetFileCreateTime());//fallback
	}
	std::string adddata = "computername=" + computername + "&username=" + username + "&mac=" + mac + "&session=" + ws2s(uniqueid);
	WinHttpClient cl(DefaultProxyGetSessionURL());
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
