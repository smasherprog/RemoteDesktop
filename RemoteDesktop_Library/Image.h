#ifndef IMAGE_H
#define IMAGE_H
#include "Rect.h"
#include <vector>

namespace RemoteDesktop{
	class HBITMAP_wrapper{
	public:
		HBITMAP Bitmap = nullptr;	
		int height = 0;
		int width = 0;
		unsigned char* raw_data = nullptr;
		explicit HBITMAP_wrapper(HBITMAP s) : Bitmap(s) { }
		~HBITMAP_wrapper()
		{
			if (Bitmap != nullptr)DeleteObject(Bitmap);
		}

	};
	class Image{
	public:
		Image() {}
		Image(unsigned char* d, unsigned long s, int h, int w) : data(d), size_in_bytes(s), height(h), width(w) {}
		//these two functions are used because removing the alpha component is a 25% size decrease
		void Optimize();//this just removes the alpha component
		//gdi is optimized with 32 bit images, so convert before sending to gdi
		void UnOptimize(std::vector<int>& buffer);//this just removes the alpha component, but a buffer is needed because the image is going to grow
		
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