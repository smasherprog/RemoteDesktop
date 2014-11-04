#ifndef MOUSECOMMON_H
#define MOUSECOMMON_H
#include <Windows.h>

namespace RemoteDesktop{
	enum Mouse_Types{
		INVALID_MOUSE = -1,
		ARROW = 32512,
		IBEAM = 32513, WAIT = 32514, CROSS = 32515, UPARROW = 32516, SIZE = 32640, ICON = 32641, SIZENWSE = 32642,
		SIZENESW = 32643, SIZEWE = 32644, SIZENS = 32645, SIZEALL = 32646, NO = 32648, HAND = 32649, APPSTARTING = 32650, HELP = 32651
	};

	class Cursor_Type{
	public:
		Cursor_Type(){}
		Cursor_Type(HCURSOR h, Mouse_Types id) :HCursor(h), ID(id) {}
		HCURSOR HCursor;
		Mouse_Types ID = INVALID_MOUSE;
	};
	inline std::vector<Cursor_Type> GetSystemCursors(){
		std::vector<Cursor_Type> _System_Cursors;
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(ARROW)), ARROW));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(IBEAM)), IBEAM));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(WAIT)), WAIT));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(CROSS)), CROSS));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(UPARROW)), UPARROW));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZE)), SIZE));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(ICON)), ICON));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZENWSE)), SIZENWSE));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZENESW)), SIZENESW));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZEWE)), SIZEWE));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZENS)), SIZENS));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(SIZEALL)), SIZEALL));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(NO)), NO));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(HAND)), HAND));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(APPSTARTING)), APPSTARTING));
		_System_Cursors.push_back(Cursor_Type(LoadCursor(NULL, MAKEINTRESOURCE(HELP)), HELP));
		return _System_Cursors;
	}


}

#endif