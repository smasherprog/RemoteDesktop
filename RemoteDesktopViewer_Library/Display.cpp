#include "stdafx.h"
#include "Display.h"
#include "..\RemoteDesktop_Library\Image.h"
#include "CommonNetwork.h"
#include <algorithm>

RemoteDesktop::Display::Display(HWND hwnd, void(__stdcall * oncursorchange)(int)) : _HWND(hwnd), _OnCursorChange(oncursorchange){

	_System_Cursors = GetSystemCursors();

	CURSORINFO cinfo;
	cinfo.cbSize = sizeof(cinfo);
	if (!GetCursorInfo(&cinfo)){

		auto f = std::find_if(_System_Cursors.begin(), _System_Cursors.end(), [&](const Cursor_Type& a){
			return cinfo.hCursor == a.HCursor;
		});
		if (f != _System_Cursors.end()){
			HCursor = *f;
		}
		else DEBUG_MSG("Could find the mouse on init!");
	}
}


void RemoteDesktop::Display::Draw(HDC hdc){
	//DEBUG_MSG("Drawing");
	RECT rect;
	GetClientRect(_HWND, &rect);

	ClientToScreen(_HWND, (LPPOINT)&rect.left);
	ClientToScreen(_HWND, (LPPOINT)&rect.right);

	if (rect.bottom == 0 && rect.left == 0 && rect.right == 0 && rect.top == 0) {
		DEBUG_MSG("Exiting cannot see window");
		return;
	}

	std::lock_guard<std::mutex> lock(_DrawLock);
	
	auto hMemDC(RAIIHDC(CreateCompatibleDC(hdc)));
	auto xoffset = 0;
	for (auto a : _Images){
		if (!a) continue;
		auto hOldBitmap = (HBITMAP)SelectObject(hMemDC.get(), a->Bitmap);
	//	DEBUG_MSG("Draw % % % % %",a->Context.Index, xoffset + a->Context.XOffset, a->Context.YOffset, a->Context.Width, a->Context.Height);
		BitBlt(hdc, xoffset, a->Context.YOffset, a->Context.Width, a->Context.Height, hMemDC.get(), 0, 0, SRCCOPY);
		SelectObject(hMemDC.get(), hOldBitmap);
		xoffset += a->Context.Width;
	}

	if (GetFocus() != _HWND) {
		DrawIcon(hdc, _MousePos.left, _MousePos.top, HCursor.HCursor);
	}
}

void RemoteDesktop::Display::Add(Image& img, New_Image_Header& h) {
	DEBUG_MSG("Adding Image index %", h.Index);
	BITMAPINFO   bi;
	memset(&bi, 0, sizeof(bi));
	assert(h.Index < MAX_DISPLAYS);

	bi.bmiHeader.biSize = sizeof(bi);
	bi.bmiHeader.biWidth = img.Width;
	bi.bmiHeader.biHeight = -img.Height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = ((img.Width * bi.bmiHeader.biBitCount + 31) / 32) * 4 * img.Height;

	img.Decompress();

	std::lock_guard<std::mutex> lock(_DrawLock);

	auto hDC = GetDC(_HWND);
	void* raw_data = nullptr;
	auto t = std::make_shared<HBITMAP_wrapper>(CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &raw_data, NULL, NULL), h);

	t->raw_data = (unsigned char*)raw_data;
	assert(img.Height*img.Width * 4 == bi.bmiHeader.biSizeImage);
	memcpy(raw_data, img.get_Data(), img.size_in_bytes());

	_Images[h.Index] = t;

	ReleaseDC(_HWND, hDC);
	InvalidateRect(_HWND, NULL, false);

}
void RemoteDesktop::Display::Update(Image& img, Update_Image_Header& h){
	//DEBUG_MSG("UpdateImage");
	assert(h.Index < MAX_DISPLAYS);
	auto t = _Images[h.Index];
	if (!t) return;

	img.Decompress();
	std::lock_guard<std::mutex> lock(_DrawLock);
	Image::Copy(img, h.rect.left, h.rect.top, t->Context.Width * 4, (char*)t->raw_data, t->Context.Height, t->Context.Width);

	InvalidateRect(_HWND, NULL, false);

}
void RemoteDesktop::Display::UpdateMouse(MouseEvent_Header& h){
	_MousePos = h.pos;

	CURSORINFO cinfo;
	cinfo.cbSize = sizeof(cinfo);
	if (!GetCursorInfo(&cinfo)){
		DEBUG_MSG("Exiting cannot GetCursorInfo");
		return;
	}

	auto f = std::find_if(_System_Cursors.begin(), _System_Cursors.end(), [&](const Cursor_Type& a){
		return h.HandleID == a.ID;
	});

	if (f != _System_Cursors.end()){
		if (f->ID != HCursor.ID){
		//	DEBUG_MSG("Cursor Changed");
			_OnCursorChange(f->ID);
		}
		HCursor = *f;
	}
	InvalidateRect(_HWND, NULL, false);
}
