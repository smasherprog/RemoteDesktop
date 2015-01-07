#ifndef NETWORKPROCESSOR123_H
#define NETWORKPROCESSOR123_H
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace RemoteDesktop{
	class DesktopMonitor;
	class SocketHandler;

	class NetworkProcessor{
		std::thread _Worker;
		std::mutex _Lock, _ContainerLock;
		std::condition_variable _CV;

		bool _Running = false;

		std::vector<std::shared_ptr<SocketHandler>> _Receives;
		std::unique_ptr<DesktopMonitor> _DesktopMonitor;

		void _Run();

	public:
		NetworkProcessor();
		~NetworkProcessor();

		void ReceiveEvent(std::shared_ptr<SocketHandler>& s);

	};

}

#endif