#include "stdafx.h"
#include "RemoteDesktopViewer.h"

std::vector<HWND> Buttons;
RECT buttonrect;
bool LastButtons = false;
bool ButtonsShown = false;

void ReadjustButtons(){

	if (ButtonsShown == LastButtons) return;
	RECT r;
	GetWindowRect(_H_wnd, &r);
	auto buttonwidth = Buttons.size() * 30;
	if (buttonwidth > 0) buttonwidth = buttonwidth / 2;//mid width
	auto windowidth = r.right - r.left;
	if (windowidth > 0) windowidth = windowidth / 2;//mid width
	auto left = windowidth - buttonwidth;
	auto top = -25;
	if (ButtonsShown) top = 0;


	buttonrect.left = left;
	buttonrect.top = top;
	buttonrect.bottom = top + 30;
	for (auto a : Buttons){
		MoveWindow(a, left, top, 30, 30, TRUE);
		left += 30;
	}
	buttonrect.right = left;
	LastButtons = ButtonsShown;
}
WNDPROC  OldButtonProc1;
LRESULT CALLBACK ButtonProc1(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_MOUSEMOVE:
		ButtonsShown = true;
		ReadjustButtons();
		return 0;
	}
	return CallWindowProc(OldButtonProc1, hwnd, msg, wp, lp);
}

WNDPROC  OldButtonProc2;
LRESULT CALLBACK ButtonProc2(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{

	switch (msg) {

	case WM_MOUSEMOVE:
		ButtonsShown = true;
		ReadjustButtons();
		return 0;
	}
	return CallWindowProc(OldButtonProc2, hwnd, msg, wp, lp);
}

WNDPROC  OldButtonProc3;
LRESULT CALLBACK ButtonProc3(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_MOUSEMOVE:
		ButtonsShown = true;
		ReadjustButtons();
		return 0;
	}
	return CallWindowProc(OldButtonProc3, hwnd, msg, wp, lp);
}

void CreateButtons(){
	int top = 10;
	int left = 50;
	auto img = LoadImage(hInst, MAKEINTRESOURCE(IDB_NEWCONNECT), IMAGE_BITMAP, 25, 25, NULL);
	HWND hwndButton = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,  // Styles 
		left,         // x position 
		top,         // y position 
		30,        // Button width
		30,        // Button height
		_H_wnd,     // Parent window
		(HMENU)NEWCONNECT,       // No menu.
		hInst,
		NULL);
	SendMessage(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(img));
	OldButtonProc1 = (WNDPROC)SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc1);
	Buttons.push_back(hwndButton);
	left += 30;

	img = LoadImage(hInst, MAKEINTRESOURCE(IDB_CAD), IMAGE_BITMAP, 25, 25, NULL);
	hwndButton = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,  // Styles 
		left,         // x position 
		top,         // y position 
		30,        // Button width
		30,        // Button height
		_H_wnd,     // Parent window
		(HMENU)SENDCAD,       // No menu.
		hInst,
		NULL);
	SendMessage(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(img));
	OldButtonProc2 = (WNDPROC)SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc2);
	Buttons.push_back(hwndButton);
	left += 30;

	img = LoadImage(hInst, MAKEINTRESOURCE(IDB_DISCONNECT), IMAGE_BITMAP, 25, 25, NULL);
	hwndButton = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,  // Styles 
		left,         // x position 
		top,         // y position 
		30,        // Button width
		30,        // Button height
		_H_wnd,     // Parent window
		(HMENU)DISCONNECT,       // No menu.
		hInst,
		NULL);
	SendMessage(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(img));
	OldButtonProc3 = (WNDPROC)SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc3);
	Buttons.push_back(hwndButton);
	left += 30;
	ButtonsShown = true;
	ReadjustButtons();
}