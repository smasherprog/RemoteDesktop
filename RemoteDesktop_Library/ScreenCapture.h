#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H
#include <Windows.h>
#include <vector>
#include "Image.h"

namespace RemoteDesktop{
	class ScreenCapture{

		HWND nDesk = nullptr;
		HDC nSrce = nullptr;
		HDC nDest = nullptr;
		HBITMAP nBmp = nullptr;
		std::vector<unsigned char> WorkingBuffer;

	public:
		~ScreenCapture();
		Image GetPrimary();
		void ReleaseHandles();
	};
};



#endif