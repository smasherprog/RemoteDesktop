#ifndef VIRTUALSCREEN123_H
#define VIRTUALSCREEN123_H
#include <vector>
#include "Image.h"
#include <memory>
#include "Utilities.h"
#include "Handle_Wrapper.h"
#include "Delegate.h"

namespace RemoteDesktop{
	struct Monitor{
		int Width = 0;
		int Height = 0;
		int Depth = 0;
		wchar_t Device[32];
		int Offsetx = 0;
		int Offsety = 0;
		int Index = 0;
	};
	class Screen{
	public:
		Screen(){
			ZEROMEMORY(MonitorInfo);
		}
		std::shared_ptr<RemoteDesktop::Image> Image;
		Monitor MonitorInfo;
	}; 

	class VirtualScreen{
		std::vector<Screen> Previous;
		RAIIHDC_TYPE CaptureDC, DesktopDC;
		RAIIHBITMAP_TYPE CaptureBmp;

		bool CreateCaptureBitmap();
		void ReorderScreens();
		

	public:
		VirtualScreen();

		std::vector<Screen> Screens;
	
		//width of the total size of all displays
		static int VirtualScreenWidth;
		//total height of the virtual display
		static int VirtualScreenHeight;

		static int XOffset_to_Zero;
		static int YOffset_to_Zero;
		void clear();
		void Update();
		//take points which are in virtual space and translate to pixel screen space
		static void Map_to_ScreenSpace(long& x, long& y);

		//this will be called for each monitor if even one of the screen changes its resolution, or ordering, or position
		Delegate<void, const Screen&> OnResolutionChanged;
		Delegate<void, const Screen&, const Rect&> OnScreenChanged;
	};

}


#endif