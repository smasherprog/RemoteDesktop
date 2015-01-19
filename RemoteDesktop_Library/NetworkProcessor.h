#ifndef NETWORKPROCESSOR123_H
#define NETWORKPROCESSOR123_H
#include <thread>
#include <memory>
#include "Concurrent_Queue.h"
#include "Delegate.h"

namespace RemoteDesktop{
	class SocketHandler;
	struct Packet_Header;

	class NetworkProcessor{

		std::thread _Worker;
		Concurrent_Queue<std::shared_ptr<SocketHandler>> _Queue;

		bool _Running;
		void _Run();

		Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> _Receive_callback;
		Delegate<void, std::shared_ptr<SocketHandler>&> _Onconnect_callback;

	public:
		NetworkProcessor(Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&>& receive_callback, Delegate<void, std::shared_ptr<SocketHandler>&>& onconnect_callback);
		~NetworkProcessor();

		void Receive(std::shared_ptr<SocketHandler>& h);

	};

}

#endif