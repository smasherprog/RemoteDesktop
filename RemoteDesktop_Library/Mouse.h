#ifndef MOUSE_CAPTURING123_H
#define MOUSE_CAPTURING123_H

namespace RemoteDesktop{
	class MouseCapture{
		CURSORINFO _LastCursorInfo;
		int Last_Screen_X, Last_Screen_Y;
		int Current_Screen_X, Current_Screen_Y;


	public:
		MouseCapture();
		void Update();

	};
};
#endif