// RemoteDesktopServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\RemoteDesktopServer_Library\MainServer.h"

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int argc;
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	RemoteDesktop::Startup(argv, argc, false);
	return 0;
}

