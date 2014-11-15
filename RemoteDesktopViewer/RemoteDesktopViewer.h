#pragma once

#include "resource.h"

#define NEWCONNECT 1000
#define DISCONNECT 1001
#define SENDCAD 1002

// Global Variables:
extern HINSTANCE hInst;								// current instance
extern HWND _H_wnd;
extern bool ButtonsShown;

void CreateButtons();
void ReadjustButtons();