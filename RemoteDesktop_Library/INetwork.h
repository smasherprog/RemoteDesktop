#ifndef INETWORK123_H
#define INETWORK123_H
#include <string>
#include "CommonNetwork.h"

namespace RemoteDesktop{

	class INetwork{
	public:

		INetwork();
		virtual ~INetwork();

		virtual void Start(std::wstring port, std::wstring host) = 0;
		virtual void Stop(bool blocking) = 0;
		virtual void Send(NetworkMessages m, const NetworkMsg& msg) = 0;
		virtual bool Is_Running() const = 0;

	};

}

#endif
