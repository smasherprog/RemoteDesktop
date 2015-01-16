#include "stdafx.h"
#include "VirtualScreen.h"		
#include "Handle_Wrapper.h"
#include "Image.h"


RemoteDesktop::VirtualScreen::VirtualScreen(){

}

std::shared_ptr<RemoteDesktop::Image> CaptureDesktop(const RemoteDesktop::RAIIHDC_TYPE &desktopGuard, int left, int top);
BOOL CALLBACK RemoteDesktop::VirtualScreen::MonitorEnumProc(
	HMONITOR hMonitor,  // handle to display monitor
	HDC hdcMonitor,        // handle to monitor DC
	LPRECT lprcMonitor,   // monitor intersection rectangle
	LPARAM dwData         // data
	)
{
	auto hdcPool = reinterpret_cast<std::vector<RemoteDesktop::Screen>*>(dwData);
	auto desktopGuard(RAIIHDC(hdcMonitor));
	RECT rect = *lprcMonitor;
	RemoteDesktop::Screen s;
	s.XOffset = lprcMonitor->left;
	s.YOffset = lprcMonitor->top;
	s.Image = CaptureDesktop(desktopGuard, lprcMonitor->left, lprcMonitor->top);
	hdcPool->push_back(s);
	return true;
}


void RemoteDesktop::VirtualScreen::Update(){
	Previous = std::move(Current);
	auto hDesktopDC(RAIIHDC(GetDC(NULL)));
	EnumDisplayMonitors(hDesktopDC.get(), NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&Current));

	std::sort(Current.begin(), Current.end(), [](const Screen& i, const Screen& j){
		return i.XOffset < j.XOffset;
	});
	YOffset_to_Zero = 0;
	for (auto& a : Current){
		YOffset_to_Zero = std::min(Current[0].YOffset, YOffset_to_Zero);
	}
	XOffset_to_Zero = Current[0].XOffset;
	for (auto& a : Current){
		a.YOffset += -YOffset_to_Zero;
	}
}
void RemoteDesktop::VirtualScreen::Map_to_ScreenSpace(long& x, long& y){
	//auto left = GetSystemMetrics(SM_XVIRTUALSCREEN) + XOffset_to_Zero;
	//auto top = GetSystemMetrics(SM_YVIRTUALSCREEN) + YOffset_to_Zero;
	//std::cout << left << "  " << top << std::endl;
	auto xx = (float)(x + XOffset_to_Zero);
	auto yy = (float)(y + YOffset_to_Zero);
	auto vx = (float)(GetSystemMetrics(SM_CXSCREEN));
	auto vy = (float)(GetSystemMetrics(SM_CYSCREEN));
	std::cout << vx << "  " << vy << std::endl;
	x = (xx * 65535.0f) / (vx - 1.0f);
	y = (yy * 65535.0f) / (vy - 1.0f);
	std::cout << x << "  " << y << std::endl;
	std::cout <<std::endl;
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