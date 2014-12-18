#ifndef SERVERSERVICE123_H
#define SERVERSERVICE123_H

#include <memory>
#include "ServiceBase.h"
namespace RemoteDesktop{
	class ServiceMonitor;
}
class ServerService : public CServiceBase
{
public:
	
	ServerService(PWSTR pszServiceName,
		BOOL fCanStop = TRUE,
		BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = FALSE);
	virtual ~ServerService();

	virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv) override;
	virtual void OnStop() override; 
	virtual void OnShutdown() override;
	virtual void OnSessionChange(DWORD eventtype) override;

	std::unique_ptr<RemoteDesktop::ServiceMonitor> _ServiceMonitor;

};

#endif