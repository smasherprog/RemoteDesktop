#ifndef DESKTOPBACKGROUND123_H
#define DESKTOPBACKGROUND123_H
#include <string>

namespace RemoteDesktop{

	class DesktopBackground{
			std::wstring UserSID;
			std::wstring OldWallpaper;
			unsigned char Red, Green, Blue;
			bool SetColor(unsigned char red, unsigned char green, unsigned char blue);
			bool SetWallpaper(std::wstring path);
		public:
			DesktopBackground();
			~DesktopBackground();
			void Restore();
			bool Set(unsigned char red, unsigned char green, unsigned char blue);	
			bool Set(std::wstring path);
		};
	
}

#endif