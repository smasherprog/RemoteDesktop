#ifndef REMOTEDISPLAY123_H
#define REMOTEDISPLAY123_H
#include <Windows.h>
#include <memory>
#include <mutex>
#include "MouseCommon.h"
#include "Rect.h"

namespace RemoteDesktop{
	class HBITMAP_wrapper;
	class Image;
	struct Image_Diff_Header;
	struct MouseEvent_Header;

	class Display{

		std::unique_ptr<HBITMAP_wrapper> _HBITMAP_wrapper;
		HWND _HWND;
		std::mutex _DrawLock;

		std::vector<Cursor_Type> _System_Cursors;

		Point _MousePos;
	
	public:
		Display(HWND hwnd);

		void NewImage(Image& img);
		void UpdateImage(Image& img, Image_Diff_Header& h);
		void UpdateMouse(MouseEvent_Header& h);
		void Draw(HDC hdc);

		Cursor_Type HCursor;
	};

};

#endif