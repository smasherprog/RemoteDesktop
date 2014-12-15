#ifndef IMAGE_H
#define IMAGE_H
#include "Rect.h"
#include <vector>

namespace RemoteDesktop{
	namespace Image_Settings{
		extern int Quality;
		extern bool GrazyScale;
	}
	class Image{
	public:

		Image() {}
		Image(unsigned char* d, unsigned long s, int h, int w) : data(d), size_in_bytes(s), height(h), width(w) {}

		void Compress();
		void Decompress(std::vector<int>& buffer);
		Image Clone(std::vector<char>& buffer) const;

		unsigned char* data = nullptr;
		unsigned long size_in_bytes = 0;
		int height = 0;
		int width = 0;
		int stride = 4;



		static Rect Difference(Image first, Image second);
		static Image Copy(Image src_img, Rect r, std::vector<unsigned char>& buffer);
		static void Copy(Image src_img, int dst_left, int dst_top, int dst_stride, unsigned char* dst, int dst_height, int dst_width);
		

	};

};
#endif