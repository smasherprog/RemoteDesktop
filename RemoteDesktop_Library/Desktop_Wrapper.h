#ifndef DESKTOP_WRAPPER123_h
#define DESKTOP_WRAPPER123_h

namespace RemoteDesktop{
	class Desktop_Wrapper{
		HDESK Handle = NULL;
	public:
		Desktop_Wrapper(HDESK h) : Handle(h){}
		~Desktop_Wrapper();
		HDESK get_Handle() const { return Handle; }
	};
}

#endif