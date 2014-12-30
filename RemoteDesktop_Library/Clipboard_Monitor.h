#ifndef CLIPBOARDMONITOR123_H
#define CLIPBOARDMONITOR123_H
#include <thread>
#include "Clipboard.h"
#include "Delegate.h"
#include <mutex>

namespace RemoteDesktop{
	class ClipboardMonitor{
		void _Run();
		std::thread _BackGroundWorker;
		std::mutex _ClipboardLock;
		bool _Running = false;
		HWND _Hwnd = NULL;
		Clipboard_Data _Clipboard_Data;
		Delegate<void, const Clipboard_Data&> _OnClipboardChanged;
		bool _IgnoreClipUpdateNotice = false;
		bool _ShareClipboard = true;

	public:
		ClipboardMonitor(Delegate<void, const Clipboard_Data&> c);
		~ClipboardMonitor();
		void Restore(const Clipboard_Data& c);
		void set_ShareClipBoard(bool s);
	};
}

#endif