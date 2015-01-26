#ifndef MOUSE_CAPTURING123_H
#define MOUSE_CAPTURING123_H
#include "Rect.h"
#include "MouseCommon.h"
#include "..\RemoteDesktop_Library\Delegate.h"

namespace RemoteDesktop{
	struct MouseEvent_Header;

	class MouseCapture{
		
		CURSORINFO _LastCursorInfo;
		std::vector<Cursor_Type> _System_Cursors;
		static Point Last_ScreenPos;
		static Mouse_Types Last_Mouse;

	public:

		static Mouse_Types Current_Mouse;
		static Point Current_ScreenPos;

		MouseCapture();
		void Update();

		static void Update(MouseEvent_Header& h);

		Delegate<void, const MouseCapture&> OnMouseChanged;
	};
};
#endif