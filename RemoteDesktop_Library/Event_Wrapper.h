#ifndef EVENT_WRAPPER123_h
#define EVENT_WRAPPER123_h

namespace RemoteDesktop{
	class Event_Wrapper{
		void* Handle = NULL;
	public:
		Event_Wrapper(void* h) : Handle(h){}
		~Event_Wrapper();
		void* get_Handle() const { return Handle;  }
	};
}

#endif