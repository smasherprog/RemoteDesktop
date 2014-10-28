#ifndef DEBUGCONSOLE_H
#define DEBUGCONSOLE_H
#include <stdio.h>

namespace RemoteDesktop{
	class CConsole {
		_iobuf m_OldStdin, m_OldStdout;
		bool m_OwnConsole;
	public:
		CConsole();
		~CConsole();
	};
}

#endif