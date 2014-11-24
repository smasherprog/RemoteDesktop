#include "stdafx.h"
#include "ScreenCapture.h"

RemoteDesktop::ScreenCapture::~ScreenCapture(){
	ReleaseHandles();
}
void RemoteDesktop::ScreenCapture::ReleaseHandles(){
	if (nSrce != NULL) ReleaseDC(nDesk, nSrce);
	if (nDest != NULL) DeleteDC(nDest);
	if (nBmp != NULL) DeleteObject(nBmp);
	nDesk = NULL;
	nSrce = NULL;
	nDest = NULL;
	nBmp = NULL;
}

//use the buffer passed in
RemoteDesktop::Image RemoteDesktop::ScreenCapture::GetPrimary(std::vector<unsigned char>& buffer){

	auto screenw(GetSystemMetrics(SM_CXSCREEN)), screenh(GetSystemMetrics(SM_CYSCREEN));
	if (nDesk == NULL)
		nDesk = GetDesktopWindow();
	if (nSrce == NULL)
		nSrce = GetDC(nDesk);
	if (nDest == NULL)
		nDest = CreateCompatibleDC(nSrce);
	if (nBmp == NULL)
		nBmp = CreateCompatibleBitmap(nSrce, screenw, screenh);

	auto oldbmp = SelectObject(nDest, nBmp);

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
	SelectObject(nDest, oldbmp);
	//ReleaseHandles();
	return Image(buffer.data(), dwBmpSize, screenh, screenw, false);
}