#include "stdafx.h"
#include "EventLog.h"
#include <string>
std::unique_ptr<RemoteDesktop::EventLog> RemoteDesktop::INTERNAL::_Logging;

RemoteDesktop::EventLog::~EventLog(){
	DeregisterEventSource(_EventSource);
}
RemoteDesktop::EventLog::EventLog(std::wstring name){
	Name = name;
	auto key_path(L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" + name);

	HKEY key;

	DWORD last_error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		key_path.c_str(),
		0,
		0,
		REG_OPTION_NON_VOLATILE,
		KEY_SET_VALUE,
		0,
		&key,
		0);
	if (ERROR_SUCCESS == last_error)
	{
		wchar_t szPath[MAX_PATH];
		bool ret = false;

		GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));

		const DWORD types_supported = EVENTLOG_ERROR_TYPE |
			EVENTLOG_WARNING_TYPE |
			EVENTLOG_INFORMATION_TYPE;

		RegSetValueEx(key,
			L"EventMessageFile",
			0,
			REG_SZ,
			(BYTE*)szPath,
			wcsnlen_s(szPath, MAX_PATH) * 2);

		RegSetValueEx(key,
			L"CategoryMessageFile",
			0,
			REG_SZ,
			(BYTE*)szPath,
			wcsnlen_s(szPath, MAX_PATH) * 2);

		RegSetValueEx(key,
			L"ParameterMessageFile",
			0,
			REG_SZ,
			(BYTE*)szPath,
			wcsnlen_s(szPath, MAX_PATH) * 2);

		RegSetValueEx(key,
			L"TypesSupported",
			0,
			REG_DWORD,
			(LPBYTE)&types_supported,
			sizeof(types_supported));
		DWORD catcount = 3;
		RegSetValueEx(key,
			L"CategoryCount",
			0,
			REG_DWORD,
			(LPBYTE)&catcount,
			sizeof(catcount));
		RegCloseKey(key);
	}
	else
	{
		std::cerr << "Failed to install source: " << last_error << "\n";
	}

	_EventSource = RegisterEventSource(NULL, name.c_str());
}

void RemoteDesktop::EventLog::Init(std::wstring name){
	RemoteDesktop::INTERNAL::_Logging = std::make_unique<RemoteDesktop::EventLog>(name);
}	

void RemoteDesktop::EventLog::WriteLog(std::wstring msg, EventType wType, EventCategory categoryid, EventID eventid){
	if (RemoteDesktop::INTERNAL::_Logging) RemoteDesktop::INTERNAL::_Logging->Write(msg, wType, categoryid, eventid);
}
void RemoteDesktop::EventLog::WriteLog(std::vector<std::wstring> msgs, EventType wType, EventCategory categoryid, EventID eventid){
	if (RemoteDesktop::INTERNAL::_Logging) RemoteDesktop::INTERNAL::_Logging->Write(msgs, wType, categoryid, eventid);
}
void RemoteDesktop::EventLog::Write(std::wstring msg, EventType wType, EventCategory categoryid, EventID eventid){
	LPCWSTR lpszStrings[2] = { NULL, NULL };
	if (_EventSource)
	{
		lpszStrings[0] = Name.c_str();
		lpszStrings[1] = msg.c_str();

		ReportEvent(_EventSource,  // Event log handle
			wType,                 // Event type
			categoryid,                     // Event category
			eventid,                     // Event identifier
			NULL,                  // No security identifier
			2,                     // Size of lpszStrings array
			0,                     // No binary data
			lpszStrings,           // Array of strings
			NULL                   // No binary data
			);
	}
}
void RemoteDesktop::EventLog::Write(std::vector<std::wstring> msgs, EventType wType, EventCategory categoryid, EventID eventid){

	std::vector<const wchar_t*> strs;
	strs.push_back(Name.c_str());
	for (auto& a : msgs) strs.push_back(a.c_str());

	if (_EventSource)
	{
		ReportEvent(_EventSource,  // Event log handle
			wType,                 // Event type
			categoryid,                     // Event category
			eventid,                     // Event identifier
			NULL,                  // No security identifier
			strs.size(),			// Size of lpszStrings array
			0,                     // No binary data
			strs.data(),           // Array of strings
			NULL                   // No binary data
			);
	}
}
