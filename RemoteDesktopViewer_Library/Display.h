#ifndef REMOTEDISPLAY123_H
#define REMOTEDISPLAY123_H
#include <Windows.h>
#include <memory>
#include <mutex>
#include "MouseCommon.h"
#include "Rect.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"

namespace RemoteDesktop{
	class HBITMAP_wrapper{
	public:
		HBITMAP Bitmap = nullptr;
		int height = 0;
		int width = 0;
		unsigned char* raw_data = nullptr;
		explicit HBITMAP_wrapper(HBITMAP s) : Bitmap(s) { }
		~HBITMAP_wrapper()
		{
			if (Bitmap != nullptr)DeleteObject(Bitmap);
		}

	};
	class Image;
	struct Image_Diff_Header;
	struct MouseEvent_Header;

	class Display{

		std::unique_ptr<HBITMAP_wrapper> _HBITMAP_wrapper;
		HWND _HWND;
		std::mutex _DrawLock;
	
		std::vector<Cursor_Type> _System_Cursors;

		Point _MousePos;
		Cursor_Type HCursor;
		void(__stdcall * _OnCursorChange)(int c_type);

	public:
		Display(HWND hwnd, void(__stdcall * oncursorchange)(int));

		void NewImage(Image& img);
		void UpdateImage(Image& img, Rect& h);
		void UpdateMouse(MouseEvent_Header& h);
		void Draw(HDC hdc);
		//bool SetCursor();

		
	};

};

#endif