#ifndef VIRTUALSCREEN123_H
#define VIRTUALSCREEN123_H
#include <vector>
#include "Image.h"
#include <memory>

namespace RemoteDesktop{
	struct Screen{
		std::shared_ptr<RemoteDesktop::Image> Image;
		int XOffset, YOffset;
	};
	class VirtualScreen{
		BOOL static CALLBACK MonitorEnumProc(
			HMONITOR hMonitor,  // handle to display monitor
			HDC hdcMonitor,        // handle to monitor DC
			LPRECT lprcMonitor,   // monitor intersection rectangle
			LPARAM dwData         // data
			);
	public:
		VirtualScreen();

		std::vector<Screen> Current, Previous;
		int XOffset_to_Zero;
		int YOffset_to_Zero;
		//this will reorder
		void Update();
		//take points which are in virtual space and translate to pixel screen space
		void Map_to_ScreenSpace(long& x, long& y);

	};

}


#endif