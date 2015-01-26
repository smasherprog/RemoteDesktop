#include "stdafx.h"
#include "VirtualScreen.h"		
#include "Handle_Wrapper.h"
#include "Image.h"
#include <algorithm>

int RemoteDesktop::VirtualScreen::VirtualScreenWidth = 0;
int RemoteDesktop::VirtualScreen::VirtualScreenHeight = 0;
int RemoteDesktop::VirtualScreen::XOffset_to_Zero = 0;
int RemoteDesktop::VirtualScreen::YOffset_to_Zero = 0;

std::vector<RemoteDesktop::Screen> GetMoitors(){
	std::vector<RemoteDesktop::Screen> ret;
	DISPLAY_DEVICE dd;
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	for (auto i = 0; EnumDisplayDevices(NULL, i, &dd, 0); i++){
		RemoteDesktop::Screen temp;

		//monitor must be attached to desktop and not a mirroring device
		if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) & !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)){

			wcsncpy_s(temp.MonitorInfo.Device, dd.DeviceName, ARRAYSIZE(dd.DeviceName));
		}
		else continue;//not a valid monitor
		DEVMODE devMode;
		devMode.dmSize = sizeof(devMode);
		EnumDisplaySettings(temp.MonitorInfo.Device, ENUM_CURRENT_SETTINGS, &devMode);
		temp.MonitorInfo.Offsetx = devMode.dmPosition.x;
		temp.MonitorInfo.Offsety = devMode.dmPosition.y;
		temp.MonitorInfo.Width = devMode.dmPelsWidth;
		temp.MonitorInfo.Height = devMode.dmPelsHeight;
		temp.MonitorInfo.Depth = devMode.dmBitsPerPel;
		ret.push_back(temp);
	}
	return ret;
}

RemoteDesktop::VirtualScreen::VirtualScreen() : DesktopDC(RAIIHDC(nullptr)), CaptureDC(RAIIHDC(nullptr)), CaptureBmp(RAIIHBITMAP(nullptr)){

}

bool AnyChanges(std::vector<RemoteDesktop::Screen>& left, std::vector<RemoteDesktop::Screen>& right){
	if (left.size() != right.size()) return true;
	for (auto i = 0; i < left.size(); i++){
		if (memcmp(&(left[i].MonitorInfo), &(right[i].MonitorInfo), sizeof(right[i].MonitorInfo)) != 0) return true;
	}
	return false;
}

void RemoteDesktop::VirtualScreen::Map_to_ScreenSpace(long& x, long& y){
	//auto left = GetSystemMetrics(SM_XVIRTUALSCREEN) + XOffset_to_Zero;
	//auto top = GetSystemMetrics(SM_YVIRTUALSCREEN) + YOffset_to_Zero;
	auto xx = (float)(x + XOffset_to_Zero);
	auto yy = (float)(y + YOffset_to_Zero);
	auto vx = (float)(GetSystemMetrics(SM_CXSCREEN));
	auto vy = (float)(GetSystemMetrics(SM_CYSCREEN));
	x = (xx * 65535.0f) / (vx - 1.0f);
	y = (yy * 65535.0f) / (vy - 1.0f);

}

void RemoteDesktop::VirtualScreen::clear(){
	VirtualScreenWidth = VirtualScreenHeight = XOffset_to_Zero = YOffset_to_Zero = 0;
	Previous.clear();
	Screens.clear();

	CaptureDC = nullptr;
	DesktopDC = nullptr;
	CaptureBmp = nullptr; 
	DEBUG_MSG("VirtualScreen::clear(). . ");
}
void RemoteDesktop::VirtualScreen::ReorderScreens(){
	XOffset_to_Zero = GetSystemMetrics(SM_XVIRTUALSCREEN);
	YOffset_to_Zero = GetSystemMetrics(SM_YVIRTUALSCREEN);
	if (Screens.empty()) return;//could not capture monitors.. get out

	//organize the monitors so that the ordering is left to right for displaying purposes
	std::sort(Screens.begin(), Screens.end(), [](const Screen& i, const Screen& j){
		return i.MonitorInfo.Offsetx < j.MonitorInfo.Offsetx;
	});
	for (auto i = 0; i < Screens.size(); i++) Screens[i].MonitorInfo.Index = i;
	/*for (auto& a : Screens){
		YOffset_to_Zero = std::min(Screens[0].MonitorInfo.Offsety, YOffset_to_Zero);
	}
	XOffset_to_Zero = Screens[0].MonitorInfo.Offsetx;
	for (auto& a : Screens){
		a.MonitorInfo.Offsety += -YOffset_to_Zero;
	}*/
}
//needs to be the largest of the monitors
bool RemoteDesktop::VirtualScreen::CreateCaptureBitmap(){
	int width = 0;
	int height = 0;
	for (auto& a : Screens){
		width = std::max(a.MonitorInfo.Width, width);
		height = std::max(a.MonitorInfo.Height, height);
	}
	if (width != 0 && height != 0){
		CaptureBmp = RAIIHBITMAP(CreateCompatibleBitmap(DesktopDC.get(), width, height));
		return true;
	}
	else CaptureBmp = nullptr;
	return false;
}
std::shared_ptr<RemoteDesktop::Image> CaptureDesktop(HDC desktop, HDC capturedc, HBITMAP bitmap, int left, int top, int width, int height);
void RemoteDesktop::VirtualScreen::Update(){
	
	bool changed = false;//this is used to determine whether I need to rebuild any of the DC's or bitmaps
	int temp = VirtualScreenWidth;
	VirtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	changed |= temp != VirtualScreenWidth;

	temp = VirtualScreenHeight;
	VirtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	changed |= temp != VirtualScreenHeight;

	Previous = std::move(Screens);
	Screens = GetMoitors();
	if (Screens.empty()) return clear();//could not capture monitors.. get out
	ReorderScreens();
	changed |= AnyChanges(Previous, Screens);
	//update and create HDCs and bitmaps for capturing
	if (changed) {
		DEBUG_MSG("SCREEN CHANGE DETECTED.. Updating . . ");
		DesktopDC = RAIIHDC(CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL));
		if (!DesktopDC)  return clear();//could not capture monitors.. get out
		if (DesktopDC.get() == nullptr) return clear();
		CaptureDC = RAIIHDC(CreateCompatibleDC(DesktopDC.get()));
		if (!CaptureDC)  return clear();//could not capture monitors.. get out
		if (CaptureDC.get() == nullptr) return clear();
		if (!CreateCaptureBitmap()) return clear();
	}
	auto t = Timer(true);
	for (auto& a : Screens){
		a.Image = CaptureDesktop(DesktopDC.get(), CaptureDC.get(), CaptureBmp.get(), a.MonitorInfo.Offsetx, a.MonitorInfo.Offsety, a.MonitorInfo.Width, a.MonitorInfo.Height);
		//t.Stop();
		//DEBUG_MSG("1) Time to Update %", t.Elapsed_milli());
		//t.Start();
	}
	//t.Stop();

	if (changed && OnResolutionChanged){
		for (auto& a : Screens) OnResolutionChanged(a);
	}
	else if (OnScreenChanged){
		auto m = std::min(Screens.size(), Previous.size());
		//t.Start();
		for (auto i = 0; i < m; i++){
			auto rect = Image::Difference(*Previous[i].Image, *Screens[i].Image);
	/*		t.Stop();
			DEBUG_MSG("2) Time to Update %", t.Elapsed_milli());
			t.Start();
			*/
			if (rect.height != 0 && rect.width != 0){
				OnScreenChanged(Screens[i], rect);
			}
		}
	}
}

std::shared_ptr<RemoteDesktop::Image> CaptureDesktop(HDC desktop, HDC capturedc, HBITMAP bitmap, int left, int top, int width, int height)
{
	// Selecting an object into the specified DC 
	auto originalBmp = SelectObject(capturedc, bitmap);

	if (!BitBlt(capturedc, 0, 0, width, height, desktop, left, top, SRCCOPY | CAPTUREBLT)){
		auto p(std::make_shared<RemoteDesktop::Image>(height, width));
		memset(p->get_Data(), 1, p->size_in_bytes());
		SelectObject(capturedc, originalBmp);
		return p;
	}

	BITMAPINFOHEADER   bi;
	memset(&bi, 0, sizeof(bi));

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	bi.biSizeImage = ((width * bi.biBitCount + 31) / 32) * 4 * height;

	auto ptr(std::make_shared<RemoteDesktop::Image>(height, width));

	GetDIBits(desktop, bitmap, 0, (UINT)height, ptr->get_Data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS);
	SelectObject(capturedc, originalBmp);

	//
	//auto lf = std::string("c:\\users\\scott\\desktop\\outfile");
	//lf += std::to_string(left);
	//lf += ".bmp";
	//ptr->Save(lf);
	//
	return ptr;
}
