// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if _DEBUG

	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <winsock2.h>
#include <Ws2tcpip.h>
// Windows Header Files:
#include <windows.h>
#include "Windowsx.h"
#include <tchar.h>

#pragma comment(lib, "Ws2_32.lib")

#include "Timer.h"
#include "Utilities.h"
#include <vector>
#include <string>
#include <memory>

