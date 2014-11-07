#ifndef SERVERSERVICE123_H
#define SERVERSERVICE123_H

#include <memory>
#include "ServiceBase.h"
namespace RemoteDesktop{
	class Server;
}
class ServerService : public CServiceBase
{
public:

	ServerService(PWSTR pszServiceName,
		BOOL fCanStop = TRUE,
		BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = FALSE);
	virtual ~ServerService(void);

	virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);
	virtual void OnStop();

	std::unique_ptr<RemoteDesktop::Server> _Server;

};

#endif