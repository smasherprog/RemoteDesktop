#ifndef IMAGE_H
#define IMAGE_H
namespace RemoteDesktop{
	class Image{
	public:
		Image(unsigned char* d, unsigned long s, int h, int w, bool c) : data(d), size_in_bytes(s), height(h), width(w),compressed(c) {}
		unsigned char* data = nullptr;
		unsigned long size_in_bytes = 0;
		int height = 0;
		int width = 0;
		bool compressed = false;
	};
};
#endif