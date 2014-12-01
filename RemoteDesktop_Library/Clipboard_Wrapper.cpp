#include "stdafx.h"
#include "Clipboard_Wrapper.h"

RemoteDesktop::Clipboard_Wrapper::Clipboard_Wrapper(void* h){
	_Valid =OpenClipboard((HWND)h);
}
RemoteDesktop::Clipboard_Wrapper::~Clipboard_Wrapper(){
	if (_Valid) CloseClipboard();
}
