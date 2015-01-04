#include "stdafx.h"
#include "Desktop_Capture_Container.h"
#include "Handle_Wrapper.h"
#include "Image.h"

std::shared_ptr<RemoteDesktop::Image> CaptureDesktop(const RemoteDesktop::RAIIHDC_TYPE &desktopGuard, int left, int top);

BOOL CALLBACK MonitorEnumProc(
	HMONITOR hMonitor,  // handle to display monitor
	HDC hdcMonitor,        // handle to monitor DC
	LPRECT lprcMonitor,   // monitor intersection rectangle
	LPARAM dwData         // data
	)
{
	auto hdcPool = reinterpret_cast<std::vector<std::shared_ptr<RemoteDesktop::Image>>*>(dwData);
	auto desktopGuard(RAIIHDC(hdcMonitor));
	RECT rect = *lprcMonitor;
	hdcPool->push_back(CaptureDesktop(desktopGuard, lprcMonitor->left, lprcMonitor->top));	
	return true;
}

std::vector<std::shared_ptr<RemoteDesktop::Image>> RemoteDesktop::CaptureDesktops(){
	auto hDesktopDC(RAIIHDC(GetDC(NULL)));
	std::vector<std::shared_ptr<Image>> imgs;
	EnumDisplayMonitors(hDesktopDC.get(), NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&imgs));
	return imgs;
}


std::shared_ptr<RemoteDesktop::Image> CaptureDesktop(const RemoteDesktop::RAIIHDC_TYPE &desktopGuard, int left, int top)
{

	auto nScreenWidth = GetDeviceCaps(desktopGuard.get(), HORZRES);
	auto nScreenHeight = GetDeviceCaps(desktopGuard.get(), VERTRES);

	auto hCaptureDC = RAIIHDC(CreateCompatibleDC(desktopGuard.get()));
	auto hCaptureBmp = RAIIHBITMAP(CreateCompatibleBitmap(desktopGuard.get(), nScreenWidth, nScreenHeight));

	// Selecting an object into the specified DC 
	auto originalBmp = SelectObject(hCaptureDC.get(), hCaptureBmp.get());
	
	if (!BitBlt(hCaptureDC.get(), 0, 0, nScreenWidth, nScreenHeight, desktopGuard.get(), left, top, SRCCOPY | CAPTUREBLT)){
		auto p(std::make_shared<RemoteDesktop::Image>(nScreenHeight, nScreenWidth));
		memset(p->get_Data(), 1, p->size_in_bytes());
		return p;
	}

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

	auto ptr(std::make_shared<RemoteDesktop::Image>(nScreenHeight, nScreenWidth));

	GetDIBits(desktopGuard.get(), hCaptureBmp.get(), 0, (UINT)nScreenHeight, ptr->get_Data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS);
	SelectObject(hCaptureDC.get(), originalBmp);	

	return ptr;
}