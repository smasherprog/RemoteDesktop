#include "stdafx.h"
#include "INetwork.h"
#include "NetworkSetup.h"

RemoteDesktop::INetwork::INetwork(){
	StartupNetwork();
}
RemoteDesktop::INetwork::~INetwork(){
	ShutDownNetwork();
}