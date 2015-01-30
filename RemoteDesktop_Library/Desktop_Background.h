#ifndef DESKTOPBACKGROUND123_H
#define DESKTOPBACKGROUND123_H
#include <string>

namespace RemoteDesktop{

	class DesktopBackground{
			std::wstring UserSID;

			wchar_t _Wallpaper_Style[12];
			wchar_t _Wallpaper_Path[1024];
			void _SaveWallpaperStyle();
			void _RestoreWallpaperStyle();
			bool _SaveWallpaper();
			void _RestoreWallpaper();

			bool _HidingWallpaper = false;
			bool _HidingActiveDesktop = false;
		public:
			DesktopBackground();
			~DesktopBackground();
			void Restore();
			void HideWallpaper();
		};
	
}

#endif