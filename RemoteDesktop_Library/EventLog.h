#ifndef EVENTLOG123_H
#define EVENTLOG123_H
#include <string>
#include <memory>

namespace RemoteDesktop{

	class EventLog{
		void* _EventSource = nullptr;
		std::wstring Name;
	public:
		enum EventType{
			INFORMATIONAL = EVENTLOG_INFORMATION_TYPE,
			WARNING = EVENTLOG_WARNING_TYPE,
			ERR = EVENTLOG_ERROR_TYPE
		};
		enum EventCategory{
			NETWORK_CATEGORY = 1
		};	
		enum EventID{
			SERVICE =100,
			CONNECT = 103,
			DISCONNECT = 104
		};
		~EventLog();
		EventLog(std::wstring name);
		void Write(std::wstring msg, EventType wType, EventCategory categoryid, EventID eventid);
		void Write(std::vector<std::wstring> msgs, EventType wType, EventCategory categoryid, EventID eventid);

		static void Init(std::wstring name);
		static void WriteLog(std::wstring msg, EventType wType, EventCategory categoryid, EventID eventid);
		static void WriteLog(std::vector<std::wstring> msgs, EventType wType, EventCategory categoryid, EventID eventid);
	};	
	namespace INTERNAL{
		extern std::unique_ptr<EventLog> _Logging;
	}
}


#endif