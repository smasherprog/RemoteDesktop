#include "stdafx.h"
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include "Ping.h"
#include "NetworkSetup.h"

bool RemoteDesktop::Ping(wchar_t* ip){
StartupNetwork();
	char SendData[32] = "Data Buffer";
	
	auto hIcmpFile = IcmpCreateFile();
	auto ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	auto ReplyBuffer = (VOID*)malloc(ReplySize);

	struct addrinfoW *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));

	IPAddr ip_addr;
	auto iResult = GetAddrInfoW(ip, NULL, &hints, &result);
	
	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		auto so = (struct sockaddr_in *) ptr->ai_addr;
		if (ptr->ai_family == AF_INET){
			memcpy(&ip_addr, &so->sin_addr, sizeof(IN_ADDR));
			break;
		}
	}

	FreeAddrInfoW(result);

	auto dwRetVal = IcmpSendEcho(hIcmpFile, ip_addr, SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000); 
	if (dwRetVal != 0) return true;
	else return false;
}