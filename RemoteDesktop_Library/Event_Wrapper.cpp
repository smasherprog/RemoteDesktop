#include "stdafx.h"
#include "Event_Wrapper.h"

RemoteDesktop::Event_Wrapper::~Event_Wrapper(){
	if (Handle!=NULL) CloseHandle(Handle);
}