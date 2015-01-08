#ifndef EVENTLOG123_H
#define EVENTLOG123_H
#include <string>
#include <memory>

namespace RemoteDesktop{

	class EventLog{
		void* _EventSource = nullptr;
		std::wstring Name;
	public:
		
		~EventLog();
		EventLog(std::wstring name);
		void Write(std::wstring msg, WORD wType = EVENTLOG_INFORMATION_TYPE);
		void Write(std::vector<std::wstring> msgs, WORD wType = EVENTLOG_INFORMATION_TYPE);

		static void Init(std::wstring name);
		static void WriteLog(std::wstring msg, WORD wType = EVENTLOG_INFORMATION_TYPE);
		static void WriteLog(std::vector<std::wstring> msgs, WORD wType = EVENTLOG_INFORMATION_TYPE);
	};	
	namespace INTERNAL{
		extern std::unique_ptr<EventLog> _Logging;
	}
}


#endif