#ifndef BASECLIENT_H
#define BASECLIENT_H
#include "CommonNetwork.h"
#include <thread>
#include <memory>
#include "..\RemoteDesktop_Library\Delegate.h"

namespace RemoteDesktop{
	class SocketHandler;

	class BaseClient{
		bool _Connect();

		void _OnDisconnectHandler(SocketHandler* socket);
		void _OnReceiveHandler(Packet_Header* p, const char* d, SocketHandler* s);
		void _OnConnectHandler(SocketHandler* socket);

		Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> Receive_CallBack;
		Delegate<void, std::shared_ptr<SocketHandler>&> Connected_CallBack;
		Delegate<void> Disconnect_CallBack;
		void(__stdcall * _OnConnectingAttempt)(int attempt, int maxattempts);

		void _Run();
		void _RunWrapper();
		std::thread _BackGroundNetworkWorker;

		std::wstring _Host, _Port;
	protected:
		bool Running = false;
		bool DisconnectReceived = false;
	public:
		BaseClient(Delegate<void, std::shared_ptr<SocketHandler>&> c,
			Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> r,
			Delegate<void> d, void(__stdcall * onconnectingattempt)(int, int));

		~BaseClient();

		void Connect(std::wstring host, std::wstring port = L"443");

		std::shared_ptr<SocketHandler> Socket;
		bool NetworkRunning() const { return Running; }
		void Send(NetworkMessages m, NetworkMsg& msg);
		void Stop();
		int MaxConnectAttempts = 3;

	};

};

#endif