#include "stdafx.h"
#include "Desktop_Wrapper.h"

RemoteDesktop::Desktop_Wrapper::~Desktop_Wrapper(){
	if (Handle != NULL) CloseDesktop(Handle);
}