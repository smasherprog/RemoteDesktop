// RemoteDesktopServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Server.h"
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	auto d = std::make_unique<RemoteDesktop::Server>();
	d->Listen(443);
	std::cout<<"Press any key to stop" << std::endl;
	char c;
	std::cin >> c;
	d.release();
	return 0;
}

