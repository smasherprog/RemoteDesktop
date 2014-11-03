#ifndef IMAGECOMPRESSION_H
#define IMAGECOMPRESSION_H
#include <vector>
#include "Image.h"

namespace RemoteDesktop{
	class ImageCompression{

		std::vector<unsigned char> CompressBuffer;
		std::vector<unsigned char> DecompressBuffer;
		void* JpegCompressor = nullptr;
		void* JpegDecompressor = nullptr;
	public:
		~ImageCompression();
		Image Compress(Image& input, int quality=60);
		Image Decompress(Image input);
	};
};

#endif