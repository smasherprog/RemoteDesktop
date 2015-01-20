#include "stdafx.h"
#include "Display.h"
#include "..\RemoteDesktop_Library\Image.h"
#include "CommonNetwork.h"
#include <algorithm>

RemoteDesktop::Display::Display(HWND hwnd, void(__stdcall * oncursorchange)(int)) : _HWND(hwnd), _OnCursorChange(oncursorchange),
_Font(RAIIHFONT(CreateFont(36, 20, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Times New Roman")))){
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
	if (!_UAC_Block){
		for (auto a : _Images){
			if (!a) continue;
			auto hOldBitmap = SelectObject(hMemDC.get(), a->Bitmap);
			//	DEBUG_MSG("Draw % % % % %",a->Context.Index, xoffset + a->Context.XOffset, a->Context.YOffset, a->Context.Width, a->Context.Height);
			BitBlt(hdc, xoffset, a->Context.YOffset, a->Context.Width, a->Context.Height, hMemDC.get(), 0, 0, SRCCOPY);
			SelectObject(hMemDC.get(), hOldBitmap);
			xoffset += a->Context.Width;
		}
	}
	else {//UAC is blocking.. show grayed image with text
		for (auto a : _GreyedImages){
			if (!a) continue;

			auto hOldBitmap = SelectObject(hMemDC.get(), a->Bitmap);
			//	DEBUG_MSG("Draw % % % % %",a->Context.Index, xoffset + a->Context.XOffset, a->Context.YOffset, a->Context.Width, a->Context.Height);
			BitBlt(hdc, xoffset, a->Context.YOffset, a->Context.Width, a->Context.Height, hMemDC.get(), 0, 0, SRCCOPY);
			SelectObject(hMemDC.get(), hOldBitmap);


			RECT r;
			r.left = xoffset;
			r.bottom = a->Context.Height;
			r.right = a->Context.Width;
			r.top = a->Context.YOffset + 100;
			auto tmpsel = SelectObject(hdc, _Font.get());
			//Rectangle(hdc, r.left, r.top, r.right, r.bottom);
			DrawText(hdc, L"UAC BLOCKING\n\nThe UAC is preventing the capture of the users desktop.\nThe Screen capturing will resume when the UAC Screen is exited.", -1, &r, DT_CENTER);
			SelectObject(hdc, tmpsel);
			xoffset += a->Context.Width;

		}
	}


	if (GetFocus() != _HWND) {
		DrawIcon(hdc, _MousePos.left, _MousePos.top, HCursor.HCursor);
	}
}

void RemoteDesktop::Display::Add(Image& img, New_Image_Header& h) {
	//UAC_Block = false;
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
void RemoteDesktop::Display::SetUAC_Block(){ 
	if (_UAC_Block) return;
	_UAC_Block = true;
	_CreateGreyedImages(); 
}
unsigned char GetGrayedValue(unsigned char input){
	auto disttomax = (float)(input);
	disttomax *= .5f;
	return input - (unsigned char)(disttomax);
}
void RemoteDesktop::Display::_CreateGreyedImages(){
	
	DEBUG_MSG("_CreateGreyedImages");
	for (auto a : _Images){
		if (!a) continue;
		BITMAPINFO   bi;
		memset(&bi, 0, sizeof(bi));

		bi.bmiHeader.biSize = sizeof(bi);
		bi.bmiHeader.biWidth = a->Context.Width;
		bi.bmiHeader.biHeight = -a->Context.Height;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biSizeImage = ((a->Context.Width * bi.bmiHeader.biBitCount + 31) / 32) * 4 * a->Context.Height;

		std::lock_guard<std::mutex> lock(_DrawLock);

		auto hDC = GetDC(_HWND);
		void* raw_data = nullptr;

		auto t = std::make_shared<HBITMAP_wrapper>(CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &raw_data, NULL, NULL), a->Context);
		t->raw_data = (unsigned char*)raw_data;
		auto dst = (unsigned char*)raw_data;
		for (auto h = 0; h < a->Context.Height; h++){
			for (auto w = 0; w < a->Context.Width; w++){
				auto src = a->raw_data + (((h*a->Context.Width) + w) * 4);
				*dst++ = GetGrayedValue(*src++);
				*dst++ = GetGrayedValue(*src++);
				*dst++ = GetGrayedValue(*src++);
				dst++;
			}
		}
		_GreyedImages[a->Context.Index] = t;
		ReleaseDC(_HWND, hDC);	
		InvalidateRect(_HWND, NULL, false);
	}
	DEBUG_MSG(" END _CreateGreyedImages");
}
void RemoteDesktop::Display::Update(Image& img, Update_Image_Header& h){
	_UAC_Block = false;
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
