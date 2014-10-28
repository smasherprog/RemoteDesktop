#ifndef IMAGE_H
#define IMAGE_H
#include "Rect.h"

namespace RemoteDesktop{
	class Image{
	public:
		Image() {}
		Image(unsigned char* d, unsigned long s, int h, int w, bool c) : data(d), size_in_bytes(s), height(h), width(w),compressed(c) {}
		unsigned char* data = nullptr;
		unsigned long size_in_bytes = 0;
		int height = 0;
		int width = 0;
		bool compressed = false;
		static Rect Difference(Image first, Image second, int horz_jump = 4);
		static Image Copy(Image src_img, Rect r, std::vector<unsigned char>& buffer);

	};
};
#endif