#include "stdafx.h"
#include "Mouse.h"

RemoteDesktop::MouseCapture::MouseCapture(){
	memset(&_LastCursorInfo, 0, sizeof(_LastCursorInfo));
	Last_Screen_X = Last_Screen_Y = Current_Screen_X = Current_Screen_Y = 0;
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
	Last_Screen_X = Current_Screen_X;
	Last_Screen_Y = Current_Screen_Y;

	Current_Screen_X = cursorInfo.ptScreenPos.x - iconinfo.xHotspot;
	Current_Screen_Y = cursorInfo.ptScreenPos.y - iconinfo.yHotspot;

	if (_LastCursorInfo.hCursor != cursorInfo.hCursor)
	{
		
	}
	//CreateIconIndirect
	_LastCursorInfo = cursorInfo;
}
