#ifndef CLIPBOARD_WRAPPER123_H
#define CLIPBOARD_WRAPPER123_H

namespace RemoteDesktop{
	class Clipboard_Wrapper{
		bool _Valid = false;
	public:
		explicit Clipboard_Wrapper(void* h);
		~Clipboard_Wrapper();
		bool IsValid() const{ return _Valid; }
	};

}

#endif