#ifndef PROCESSUTILS123_H
#define PROCESSUTILS123_H

namespace RemoteDesktop{
	bool IsElevated();
	bool IsUserAdmin();
	bool IsUserAdmin(HANDLE hTok);

	//command line args so the process can be restarted
	//returns true if the process successfully started elevated
	bool TryToElevate(wchar_t** argv, int argc);

}


#endif