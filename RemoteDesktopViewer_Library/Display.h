#ifndef REMOTEDISPLAY123_H
#define REMOTEDISPLAY123_H
#include <Windows.h>
#include <memory>
#include <mutex>
#include "MouseCommon.h"
#include "Rect.h"
#include "..\RemoteDesktop_Library\Handle_Wrapper.h"
#include "..\RemoteDesktop_Library\Image.h"
#include "..\RemoteDesktop_Library\CommonNetwork.h"

namespace RemoteDesktop{
	class HBITMAP_wrapper{
	public:
		HBITMAP Bitmap = nullptr;
		New_Image_Header Context;
		unsigned char* raw_data = nullptr;
		explicit HBITMAP_wrapper(HBITMAP s, New_Image_Header& c) : Bitmap(s), Context(c) { }
		~HBITMAP_wrapper()
		{
			if (Bitmap != nullptr)DeleteObject(Bitmap);
		}

	};

	struct MouseEvent_Header;

	class Display{

		std::shared_ptr<HBITMAP_wrapper> _Images[MAX_DISPLAYS];

		HWND _HWND;
		std::mutex _DrawLock;
	
		std::vector<Cursor_Type> _System_Cursors;

		Point _MousePos;
		Cursor_Type HCursor;
		void(__stdcall * _OnCursorChange)(int c_type);

	public:
		Display(HWND hwnd, void(__stdcall * oncursorchange)(int));

		void Add(Image& img, New_Image_Header& h);
		void Update(Image& img, Update_Image_Header& h);
		void UpdateMouse(MouseEvent_Header& h);
		void Draw(HDC hdc);
		//bool SetCursor();

		
	};

};

#endif