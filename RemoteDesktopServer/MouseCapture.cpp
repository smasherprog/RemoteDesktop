#include "stdafx.h"
#include "MouseCapture.h"
#include <algorithm>

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
}
