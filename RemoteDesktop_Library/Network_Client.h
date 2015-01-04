#ifndef NETWORKCLIENT123_H
#define NETWORKCLIENT123_H
#include "INetwork.h"
#include "Delegate.h"
#include <memory>
#include "SocketHandler.h"
#include <thread>

namespace RemoteDesktop{
	#define DEFAULTMAXCONNECTATTEMPTS 15

	class Network_Client : public INetwork{

		bool _Running = false;
		std::thread _BackgroundWorker;
		void _Run();
		std::wstring _Dst_Host, _Dst_Port;

		int _MaxConnectAttempts = DEFAULTMAXCONNECTATTEMPTS;


	public:
		Network_Client();

		virtual ~Network_Client() override;
		virtual void Start(std::wstring port, std::wstring host)override;
		virtual void Stop(bool blocking = false)override;

		virtual void Send(NetworkMessages m, const NetworkMsg& msg)override;

		virtual bool Is_Running() const override { return _Running;  };

		Delegate<void, Packet_Header*, const char*, std::shared_ptr<SocketHandler>&> OnReceive;
		Delegate<void, std::shared_ptr<SocketHandler>&> OnConnected;
		Delegate<void> OnDisconnect;


	};

}

#endif