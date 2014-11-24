#ifndef BASECLIENT_H
#define BASECLIENT_H
#include "CommonNetwork.h"
#include <thread>
#include <memory>

namespace RemoteDesktop{
	class SocketHandler;

	class BaseClient{
		bool _Connect();

		void _OnDisconnectHandler(SocketHandler* socket);
		void _OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s);
		void _OnConnectHandler(SocketHandler* socket);

		std::function<void(Packet_Header*, const char*, std::shared_ptr<SocketHandler>&)> Receive_CallBack;
		std::function<void(std::shared_ptr<SocketHandler>&)> Connected_CallBack;
		std::function<void()> Disconnect_CallBack;

	

		void _Run();
		void _RunWrapper(int connectattempts);
		std::thread _BackGroundNetworkWorker;

		std::wstring _Host, _Port;
	protected:
		bool Running = false;
		bool DisconnectReceived = false;
	public:
		explicit BaseClient(std::function<void(std::shared_ptr<SocketHandler>&)> c,
			std::function<void(Packet_Header*, const char*, std::shared_ptr<SocketHandler>&)> r,
			std::function<void()> d);
		~BaseClient();

		void Connect(std::wstring host, std::wstring port = L"443");

		std::shared_ptr<SocketHandler> Socket;
		bool NetworkRunning() const { return Running; }
		void Send(NetworkMessages m, NetworkMsg& msg);
		void Stop();
	};

};

#endif