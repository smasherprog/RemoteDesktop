#ifndef SERVICEBASE123_H
#define SERVICEBASE123_H

#include <windows.h>


class CServiceBase
{
public:

	static BOOL Run(CServiceBase &service);

	CServiceBase(PWSTR pszServiceName,
		BOOL fCanStop = TRUE,
		BOOL fCanShutdown = TRUE,
		BOOL fCanPauseContinue = FALSE);

	// Service object destructor. 
	virtual ~CServiceBase(void);

	// Stop the service.
	void Stop();

	// When implemented in a derived class, executes when a Start command is 
	// sent to the service by the SCM or when the operating system starts 
	// (for a service that starts automatically). Specifies actions to take 
	// when the service starts.
	virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);

	// When implemented in a derived class, executes when a Stop command is 
	// sent to the service by the SCM. Specifies actions to take when a 
	// service stops running.
	virtual void OnStop();

	// When implemented in a derived class, executes when a Pause command is 
	// sent to the service by the SCM. Specifies actions to take when a 
	// service pauses.
	virtual void OnPause();

	// When implemented in a derived class, OnContinue runs when a Continue 
	// command is sent to the service by the SCM. Specifies actions to take 
	// when a service resumes normal functioning after being paused.
	virtual void OnContinue();

	// When implemented in a derived class, executes when the system is 
	// shutting down. Specifies what should occur immediately prior to the 
	// system shutting down.
	virtual void OnShutdown();

	virtual void OnSessionChange(DWORD eventtype);
	// Set the service status and report the status to the SCM.
	void SetServiceStatus(DWORD dwCurrentState,
		DWORD dwWin32ExitCode = NO_ERROR,
		DWORD dwWaitHint = 0);


private:

	// Entry point for the service. It registers the handler function for the 
	// service and starts the service.
	static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);

	// The function is called by the SCM whenever a control code is sent to 
	// the service.
	static DWORD WINAPI ServiceCtrlHandler(_In_  DWORD dwControl,
		_In_  DWORD dwEventType,
		_In_  LPVOID lpEventData,
		_In_  LPVOID lpContext
		);


	// Start the service.
	void Start(DWORD dwArgc, PWSTR *pszArgv);

	// Pause the service.
	void Pause();

	// Resume the service after being paused.
	void Continue();

	// Execute when the system is shutting down.
	void Shutdown();

	void SessionChange();
	// The singleton service instance.
	static CServiceBase *s_service;

	// The name of the service
	PWSTR m_name;

	// The status of the service
	SERVICE_STATUS m_status;

	// The service status handle
	SERVICE_STATUS_HANDLE m_statusHandle;
};

#endif