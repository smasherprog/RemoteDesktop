// RemoteDesktopViewer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RemoteDesktopViewer.h"
#include <memory>
#include "Client.h"

#define MAX_LOADSTRING 100
#define NEWCONNECT 1000
#define DISCONNECT 1001
#define SENDCAD 1002

// Global Variables:
HINSTANCE hInst;								// current instance
HWND _H_wnd;

TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
#define IDB_BUTTON 1234


std::unique_ptr<RemoteDesktop::Client> _Client;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_REMOTEDESKTOPVIEWER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REMOTEDESKTOPVIEWER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	_Client.release();
	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REMOTEDESKTOPVIEWER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_REMOTEDESKTOPVIEWER));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

	hInst = hInstance; // Store instance handle in our global variable

	_H_wnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!_H_wnd)
	{
		return FALSE;
	}

	ShowWindow(_H_wnd, nCmdShow);
	UpdateWindow(_H_wnd);

	return TRUE;
}
#include <vector>
std::vector<HWND> Buttons;
RECT buttonrect;
bool LastButtons = false;
bool ButtonsShown = false;

void ReadjustButtons(HWND hWnd){

	if (ButtonsShown == LastButtons) return;
	RECT r;
	GetWindowRect(hWnd, &r);
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
		ReadjustButtons(_H_wnd);
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
		ReadjustButtons(_H_wnd);
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
		ReadjustButtons(_H_wnd);
		return 0;
	}
	return CallWindowProc(OldButtonProc3, hwnd, msg, wp, lp);
}


void Createbuttons(HWND hWnd, int top, int left){
	auto img = LoadImage(hInst, MAKEINTRESOURCE(IDB_NEWCONNECT), IMAGE_BITMAP, 25, 25, NULL);
	HWND hwndButton = CreateWindow(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,  // Styles 
		left,         // x position 
		top,         // y position 
		30,        // Button width
		30,        // Button height
		hWnd,     // Parent window
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
		hWnd,     // Parent window
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
		hWnd,     // Parent window
		(HMENU)DISCONNECT,       // No menu.
		hInst,
		NULL);
	SendMessage(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(img));
	OldButtonProc3 = (WNDPROC)SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc3);
	Buttons.push_back(hwndButton);
	left += 30;
	ButtonsShown = true;
	ReadjustButtons(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		Createbuttons(hWnd, 10, 50);
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case NEWCONNECT:
			_Client = std::make_unique<RemoteDesktop::Client>(hWnd);
			//_Client->Connect("127.0.0.1", "443");
			_Client->Connect("192.168.221.128", "443");
			break;
		case DISCONNECT:
			if (_Client) _Client->Stop();
			break;
		case SENDCAD:
			if (_Client) _Client->SendCAD();
			break;
			
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (_Client != nullptr){
			_Client->Draw(hdc);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_KEYUP:
		if (_Client != nullptr) _Client->KeyEvent(wParam, false);
		break;
	case WM_KEYDOWN:
		if (_Client != nullptr) _Client->KeyEvent(wParam, true);
		break;
	case WM_SETCURSOR:
		if (_Client != nullptr)
			if (_Client->SetCursor()) return 0;
		return DefWindowProc(hWnd, message, wParam, lParam);
	case WM_MOUSEMOVE:
		ButtonsShown = false;
		ReadjustButtons(hWnd);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEWHEEL:
		if (_Client != nullptr) _Client->MouseEvent(message, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	case WM_SIZE:
		ReadjustButtons(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
