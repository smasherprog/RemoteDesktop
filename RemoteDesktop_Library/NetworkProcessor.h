#ifndef NETWORKPROCESSOR123_H
#define NETWORKPROCESSOR123_H
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "Delegate.h"

namespace RemoteDesktop{
	class DesktopMonitor;
	class SocketHandler;

	class NetworkProcessor{

		std::thread _Worker;
		std::mutex _Lock, _ContainerLock;
		std::condition_variable _CV;

		bool _Running = false;

		int _Count = 0;
		std::vector<char> _ReceiveBuffer_In;
		std::vector<char> _ReceiveBuffer_Out;

		std::unique_ptr<DesktopMonitor> _DesktopMonitor;

		Delegate<void, std::vector<char>&, int> _Receive_CallBack;

		void _Run();

	public:
		NetworkProcessor(Delegate<void, std::vector<char>&, int> d);
		~NetworkProcessor();

		bool Add(std::vector<char>& buffer, int counter);

	};

}

#endif