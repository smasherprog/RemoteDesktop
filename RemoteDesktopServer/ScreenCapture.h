#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H
#include <Windows.h>
#include <vector>
#include "Image.h"

namespace RemoteDesktop{
	class ScreenCapture{

		HWND nDesk = NULL;
		HDC nSrce = NULL;
		HDC nDest = NULL;
		HBITMAP nBmp = NULL;

	public:
		~ScreenCapture();
		Image GetPrimary(std::vector<unsigned char>& buffer);
		void ReleaseHandles();
	};
};



#endif