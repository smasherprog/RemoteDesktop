#ifndef MOUSE_CAPTURING123_H
#define MOUSE_CAPTURING123_H
#include "Rect.h"
#include "MouseCommon.h"

namespace RemoteDesktop{

	class MouseCapture{
		
		CURSORINFO _LastCursorInfo;
		std::vector<Cursor_Type> _System_Cursors;

	public:
		Point Last_ScreenPos, Current_ScreenPos;
		Mouse_Types Last_Mouse, Current_Mouse;
		MouseCapture();
		void Update();

	};
};
#endif