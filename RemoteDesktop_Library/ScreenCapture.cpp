#include "stdafx.h"
#include "ScreenCapture.h"

RemoteDesktop::ScreenCapture::~ScreenCapture(){
	ReleaseHandles();
}
void RemoteDesktop::ScreenCapture::ReleaseHandles(){
	if (nBmp != nullptr) DeleteObject(nBmp);
	if (nDest != nullptr) DeleteDC(nDest);
	ReleaseDC(nDesk, nSrce);
	nDesk = nullptr;
	nSrce = nullptr;
	nDest = nullptr;
	nBmp = nullptr;
}

RemoteDesktop::Image RemoteDesktop::ScreenCapture::GetPrimary(){
	return GetPrimary(WorkingBuffer);
}
//use the buffer passed in
RemoteDesktop::Image RemoteDesktop::ScreenCapture::GetPrimary(std::vector<unsigned char>& buffer){

	auto screenw(GetSystemMetrics(SM_CXSCREEN)), screenh(GetSystemMetrics(SM_CYSCREEN));
	if (nDesk == nullptr)
		nDesk = GetDesktopWindow();
	if (nSrce == nullptr)
		nSrce = GetWindowDC(nDesk);
	if (nDest == nullptr)
		nDest = CreateCompatibleDC(nSrce);
	if (nBmp == nullptr)
		nBmp = CreateCompatibleBitmap(nSrce, screenw, screenh);

	auto hOldBmp = SelectObject(nDest, nBmp);

	auto b = BitBlt(nDest, 0, 0, screenw, screenh, nSrce, 0, 0, SRCCOPY | CAPTUREBLT);

	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = screenw;
	bi.biHeight = -screenh;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	auto dwBmpSize = ((screenw * bi.biBitCount + 31) / 32) * 4 * screenh;
	buffer.reserve(dwBmpSize);

	GetDIBits(nSrce, nBmp, 0, (UINT)screenh, buffer.data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS);

	return Image(buffer.data(), dwBmpSize, screenh, screenw, false);
}