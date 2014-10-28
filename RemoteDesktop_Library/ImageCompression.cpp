#include "stdafx.h"
#include "ImageCompression.h"
#include "libjpeg-turbo\turbojpeg.h"

RemoteDesktop::ImageCompression::~ImageCompression(){
	if (JpegCompressor != nullptr)
		tjDestroy(JpegCompressor);
	if (JpegDecompressor != nullptr)
		tjDestroy(JpegDecompressor);
}

RemoteDesktop::Image RemoteDesktop::ImageCompression::Compress(Image& input, int quality){
	if (input.size_in_bytes<1024 * 10) return Image(input.data, input.size_in_bytes, input.height, input.width, false);
	if (JpegCompressor == nullptr)
		JpegCompressor = tjInitCompress();
	long unsigned int _jpegSize = 0;
	auto maxsize = input.width * input.height * 4;
	if (CompressBuffer.capacity() < maxsize) CompressBuffer.reserve(maxsize);
	auto t = Timer(true);
	
	if (tjCompress2(JpegCompressor, CompressBuffer.data(), input.width, ((input.width * 32 + 31) / 32) * 4, input.height, TJPF_BGRX,
		&input.data, &_jpegSize, TJSAMP_444, quality, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1) DEBUG_MSG("Err msg %", tjGetErrorStr());
	t.Stop();
	DEBUG_MSG("Time Taken Compress %", std::to_string(t.Elapsed()));
	return Image(CompressBuffer.data(), _jpegSize, input.height, input.width,  true);

}
RemoteDesktop::Image RemoteDesktop::ImageCompression::Decompress(Image input){
	if (JpegDecompressor == nullptr)
		JpegDecompressor = tjInitCompress();
	long unsigned int _jpegSize = 0;
	int jpegSubsamp = 0;
	auto width = 0; 
	auto height = 0;
	auto t = Timer(true);

	if (tjDecompressHeader2(JpegDecompressor, input.data, _jpegSize, &width, &height, &jpegSubsamp) == -1) DEBUG_MSG("Err msg %", tjGetErrorStr());	
	
	auto maxsize = input.width * input.height * 4;
	if (DecompressBuffer.capacity() < maxsize) DecompressBuffer.reserve(maxsize);

	if (tjDecompress2(JpegDecompressor, input.data, _jpegSize, DecompressBuffer.data(), width, ((width * 32 + 31) / 32) * 4, height, TJPF_BGRX, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1)
		DEBUG_MSG("Err msg %", tjGetErrorStr());


	t.Stop();
	DEBUG_MSG("Time Taken Decompress %", std::to_string(t.Elapsed()));
	return Image(DecompressBuffer.data(), _jpegSize, input.height, input.width, true);
}