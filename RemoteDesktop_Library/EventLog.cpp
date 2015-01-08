#include "stdafx.h"
#include "EventLog.h"

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
		std::wstring path = szPath;

		DWORD last_error;
		const DWORD types_supported = EVENTLOG_ERROR_TYPE |
			EVENTLOG_WARNING_TYPE |
			EVENTLOG_INFORMATION_TYPE;

		last_error = RegSetValueEx(key,
			L"EventMessageFile",
			0,
			REG_SZ,
			(BYTE*)path.c_str(),
			path.size()+2);

		if (ERROR_SUCCESS == last_error)
		{
			last_error = RegSetValueEx(key,
				L"TypesSupported",
				0,
				REG_DWORD,
				(LPBYTE)&types_supported,
				sizeof(types_supported));
		}

		if (ERROR_SUCCESS != last_error)
		{
			DEBUG_MSG("Failed to install source values: %", last_error);
		}

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
void RemoteDesktop::EventLog::WriteLog(std::wstring msg, WORD wType){
	if (RemoteDesktop::INTERNAL::_Logging) RemoteDesktop::INTERNAL::_Logging->Write(msg, wType);
}
void RemoteDesktop::EventLog::WriteLog(std::vector<std::wstring> msgs, WORD wType){
	if (RemoteDesktop::INTERNAL::_Logging) RemoteDesktop::INTERNAL::_Logging->Write(msgs, wType);
}
void RemoteDesktop::EventLog::Write(std::wstring msg, WORD wType){
	LPCWSTR lpszStrings[2] = { NULL, NULL };
	if (_EventSource)
	{
		lpszStrings[0] = Name.c_str();
		lpszStrings[1] = msg.c_str();

		ReportEvent(_EventSource,  // Event log handle
			wType,                 // Event type
			0,                     // Event category
			0,                     // Event identifier
			NULL,                  // No security identifier
			2,                     // Size of lpszStrings array
			0,                     // No binary data
			lpszStrings,           // Array of strings
			NULL                   // No binary data
			);
	}
}
void RemoteDesktop::EventLog::Write(std::vector<std::wstring> msgs, WORD wType){

	std::vector<const wchar_t*> strs;
	strs.push_back(Name.c_str());
	for (auto& a : msgs) strs.push_back(a.c_str());

	if (_EventSource)
	{
		ReportEvent(_EventSource,  // Event log handle
			wType,                 // Event type
			0,                     // Event category
			0,                     // Event identifier
			NULL,                  // No security identifier
			strs.size(),                     // Size of lpszStrings array
			0,                     // No binary data
			strs.data(),           // Array of strings
			NULL                   // No binary data
			);
	}
}
