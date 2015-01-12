#ifndef INETWORK123_H
#define INETWORK123_H
#include <string>
#include "CommonNetwork.h"
#include <functional>
#include <thread>
#include <memory>


namespace RemoteDesktop{
	class SocketHandler;
	class INetwork{
#define DEFAULTMAXCONNECTATTEMPTS 15
	protected:
		bool _Running = false;
	
	public:

		INetwork();
		virtual ~INetwork();

		virtual void Start(std::wstring port, std::wstring host) = 0;
		virtual void Stop(bool blocking) = 0;
		virtual bool Is_Running() const { return _Running; };
		virtual void Set_RetryAttempts(int num_of_retry) = 0;
		virtual int Get_RetryAttempts(int num_of_retry) const = 0;
		virtual int Connection_Count() const = 0;

		virtual RemoteDesktop::Network_Return Send(RemoteDesktop::NetworkMessages m, const RemoteDesktop::NetworkMsg& msg) = 0;
		virtual RemoteDesktop::Network_Return Send(RemoteDesktop::NetworkMessages m) { RemoteDesktop::NetworkMsg msg; return Send(m, msg); }

		std::function<void(RemoteDesktop::Packet_Header*, const char*, std::shared_ptr<RemoteDesktop::SocketHandler>&)> OnReceived;
		std::function<void(std::shared_ptr<RemoteDesktop::SocketHandler>&)> OnConnected;
		std::function<void(std::shared_ptr<RemoteDesktop::SocketHandler>&)> OnDisconnect;
		std::function<void(int, int)> OnConnectingAttempt;

	};

}

#endif
