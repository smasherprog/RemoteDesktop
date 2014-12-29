#include "stdafx.h"
#include "Desktop_Capture_Container.h"
#include "Handle_Wrapper.h"
#include "Image.h"

RemoteDesktop::Image CaptureDesktop(const RemoteDesktop::RAIIHDC_TYPE &desktopGuard, int left, int top);

BOOL CALLBACK MonitorEnumProc(
	HMONITOR hMonitor,  // handle to display monitor
	HDC hdcMonitor,        // handle to monitor DC
	LPRECT lprcMonitor,   // monitor intersection rectangle
	LPARAM dwData         // data
	)
{
	auto hdcPool = reinterpret_cast<std::vector<RemoteDesktop::Image>*>(dwData);
	auto desktopGuard = RAIIHDC(hdcMonitor);
	RECT rect = *lprcMonitor;
	hdcPool->emplace_back(std::move(CaptureDesktop(desktopGuard, lprcMonitor->left, lprcMonitor->top)));	
	return true;
}

std::vector<RemoteDesktop::Image> RemoteDesktop::CaptureDesktops(){
	

	auto hDesktopDC = RAIIHDC(GetDC(NULL));
	std::vector<Image> imgs;
	EnumDisplayMonitors(hDesktopDC.get(), NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&imgs));
	return imgs;
}




RemoteDesktop::Image CaptureDesktop(const RemoteDesktop::RAIIHDC_TYPE &desktopGuard, int left, int top)
{

	auto nScreenWidth = GetDeviceCaps(desktopGuard.get(), HORZRES);
	auto nScreenHeight = GetDeviceCaps(desktopGuard.get(), VERTRES);

	auto hCaptureDC = RAIIHDC(CreateCompatibleDC(desktopGuard.get()));
	auto hCaptureBmp = RAIIHBITMAP(CreateCompatibleBitmap(desktopGuard.get(), nScreenWidth, nScreenHeight));

	// Selecting an object into the specified DC 
	auto originalBmp = SelectObject(hCaptureDC.get(), hCaptureBmp.get());
	
	BitBlt(hCaptureDC.get(), 0, 0, nScreenWidth, nScreenHeight, desktopGuard.get(), left, top, SRCCOPY | CAPTUREBLT);

	BITMAPINFOHEADER   bi;
	memset(&bi, 0, sizeof(bi));

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = nScreenWidth;
	bi.biHeight = -nScreenHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	bi.biSizeImage = ((nScreenWidth * bi.biBitCount + 31) / 32) * 4 * nScreenHeight;

	RemoteDesktop::Image retimg(4, nScreenHeight, nScreenWidth);

	GetDIBits(desktopGuard.get(), hCaptureBmp.get(), 0, (UINT)nScreenHeight, retimg.get_Data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS);
	SelectObject(hCaptureDC.get(), originalBmp);	

	return retimg;
}