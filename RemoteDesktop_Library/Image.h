#ifndef IMAGE_H
#define IMAGE_H
#include "Rect.h"
#include <vector>
#include <mutex>

namespace RemoteDesktop{
	namespace Image_Settings{
		extern int Quality;
		extern bool GrazyScale;
	}
	namespace INTERNAL{
		//improves speed when memory allocations are kept down because vector resize always does a memset on the unintialized elements
		extern std::vector<std::vector<char>> BufferCache;
		extern std::mutex BufferCacheLock;
	}
	class Image{

		std::vector<char> data;

	public:

		Image(const Image& other) = delete;
		Image() {}
		explicit Image(char* d, int h, int w) :Pixel_Stride(4), Height(h), Width(w) {
			if (!INTERNAL::BufferCache.empty()){
				std::lock_guard<std::mutex> lock(INTERNAL::BufferCacheLock);
				if (!INTERNAL::BufferCache.empty()){
					data = std::move(INTERNAL::BufferCache.back());
					INTERNAL::BufferCache.pop_back();
				}
			}
			data.resize(Pixel_Stride*Height*Width); memcpy(data.data(), d, Pixel_Stride);
		}
		explicit Image(int h, int w) : Pixel_Stride(4), Height(h), Width(w)  {
			if (!INTERNAL::BufferCache.empty()){
				std::lock_guard<std::mutex> lock(INTERNAL::BufferCacheLock);
				if (!INTERNAL::BufferCache.empty()){
					data = std::move(INTERNAL::BufferCache.back());
					INTERNAL::BufferCache.pop_back();
				}
			}
			data.resize(Pixel_Stride*Height*Width);
		}
		Image(Image&& other) :data(std::move(other.data)), Height(std::move(other.Height)), Width(std::move(other.Width)), Compressed(std::move(other.Compressed)){

		}
		Image& operator=(Image&& other){

			data = std::move(other.data);
			Height = std::move(other.Height);
			Width = std::move(other.Width);
			Compressed = std::move(other.Compressed);
		}
		~Image(){
			if (data.size() > 100){
				std::lock_guard<std::mutex> lock(INTERNAL::BufferCacheLock);
				INTERNAL::BufferCache.emplace_back(std::move(data));
			}
		}
		static Image Create_from_Compressed_Data(char* d, int size_in_bytes, int h, int w);
		void Compress();
		void Decompress();
		Image Clone() const;
		//mainly used for image validation
		void Save(std::string outfile);

		char* get_Data() { return data.data(); }
		size_t size_in_bytes() const { return data.size(); }
		int Height = 0;
		int Width = 0;
		//pixel stride
		const int Pixel_Stride = 4;
		bool Compressed = false;

		static Rect Difference(Image& first, Image& second);
		static Image Copy(Image& src_img, Rect r);
		static void Copy(Image& src_img, int dst_left, int dst_top, int dst_stride, char* dst, int dst_height, int dst_width);


	};

	void SaveBMP(BITMAPINFOHEADER bi, char* imgdata, std::string dst = "capture.bmp");
};
#endif