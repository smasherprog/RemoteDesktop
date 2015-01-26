#include "stdafx.h"
#include "MouseCapture.h"
#include <algorithm>
#include "..\RemoteDesktop_Library\CommonNetwork.h"
#include "..\RemoteDesktop_Library\VirtualScreen.h"

RemoteDesktop::Point RemoteDesktop::MouseCapture::Last_ScreenPos; 
RemoteDesktop::Point RemoteDesktop::MouseCapture::Current_ScreenPos;

RemoteDesktop::Mouse_Types RemoteDesktop::MouseCapture::Last_Mouse=Mouse_Types::INVALID_MOUSE;
RemoteDesktop::Mouse_Types RemoteDesktop::MouseCapture::Current_Mouse = Mouse_Types::INVALID_MOUSE;

RemoteDesktop::MouseCapture::MouseCapture(){
	memset(&_LastCursorInfo, 0, sizeof(_LastCursorInfo));
	Last_Mouse = Current_Mouse = Mouse_Types::INVALID_MOUSE;
	_System_Cursors = GetSystemCursors();
}
void RemoteDesktop::MouseCapture::Update(){
	CURSORINFO cursorInfo;
	cursorInfo.cbSize = sizeof(cursorInfo);
	if (!GetCursorInfo(&cursorInfo))
		return;

	if (cursorInfo.flags != CURSOR_SHOWING)//not showing anyway
		return;
	ICONINFO iconinfo;
	if (!GetIconInfo(cursorInfo.hCursor, &iconinfo)) return;

	if (iconinfo.hbmMask != NULL)
		DeleteObject(iconinfo.hbmMask);
	if (iconinfo.hbmColor != NULL)
		DeleteObject(iconinfo.hbmColor);

	Last_ScreenPos = Current_ScreenPos;

	Current_ScreenPos.left = cursorInfo.ptScreenPos.x - iconinfo.xHotspot;
	Current_ScreenPos.top = cursorInfo.ptScreenPos.y - iconinfo.yHotspot;

	if (_LastCursorInfo.hCursor != cursorInfo.hCursor)
	{
		auto f = std::find_if(_System_Cursors.begin(), _System_Cursors.end(), [&](const Cursor_Type& a){
			return cursorInfo.hCursor == a.HCursor;
		});
		if (f != _System_Cursors.end()){
			Last_Mouse = Current_Mouse;
			Current_Mouse = f->ID;
		}
	}
	_LastCursorInfo = cursorInfo;
	if ((Last_ScreenPos != Current_ScreenPos || Last_Mouse != Current_Mouse) && OnMouseChanged) OnMouseChanged(*this);
	Last_ScreenPos = Current_ScreenPos;
	Last_Mouse = Current_Mouse;
}
void RemoteDesktop::MouseCapture::Update(MouseEvent_Header& h){
	Last_ScreenPos = Current_ScreenPos = h.pos;
	INPUT inp;
	memset(&inp, 0, sizeof(inp));
	inp.type = INPUT_MOUSE;
	inp.mi.mouseData = 0;
	inp.mi.dx = h.pos.left;
	inp.mi.dy = h.pos.top;
	//map coordinates to screen space
	RemoteDesktop::VirtualScreen::Map_to_ScreenSpace(inp.mi.dx, inp.mi.dy);

	if (h.Action == WM_MOUSEMOVE) inp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	else if (h.Action == WM_LBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	else if (h.Action == WM_LBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	else if (h.Action == WM_RBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	else if (h.Action == WM_RBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	else if (h.Action == WM_MBUTTONDOWN) inp.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
	else if (h.Action == WM_MBUTTONUP) inp.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
	else if (h.Action == WM_MOUSEWHEEL) {
		inp.mi.dwFlags = MOUSEEVENTF_WHEEL;
		inp.mi.mouseData = h.wheel;
	}
	if (h.Action == WM_LBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
		//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, 0, 0);
		SendInput(1, &inp, sizeof(inp));
	}
	else if (h.Action == WM_RBUTTONDBLCLK){
		inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_RIGHTDOWN;
		//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, 0, 0);
		SendInput(1, &inp, sizeof(inp));
	}
	//DEBUG_MSG("GOt here");
	//mouse_event(inp.mi.dwFlags, inp.mi.dx, inp.mi.dy, inp.mi.mouseData, 0);
	SendInput(1, &inp, sizeof(inp));


}