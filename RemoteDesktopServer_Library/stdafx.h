// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include <windows.h>
#include "Timer.h"
#include "Utilities.h"
#include <vector>
#include <string>

#include <tchar.h>
#include "Shellapi.h"
#include <strsafe.h>

#include "resource.h"
#if _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


#endif