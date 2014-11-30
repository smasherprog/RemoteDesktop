#ifndef CLIPBOARD123_H
#define CLIPBOARD123_H
#include <thread>

namespace RemoteDesktop{
	class Clipboard{
		void _Run();
		std::thread _BackGroundWorker;
		bool _Running = false;

	public:
		Clipboard();
		~Clipboard();
	
	};
}

#endif