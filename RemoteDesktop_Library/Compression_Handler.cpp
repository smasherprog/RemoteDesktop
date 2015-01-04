#include "stdafx.h"
#include "Compression_Handler.h"
#include "lz4.h"
#include "lz4hc.h"

int RemoteDesktop::Compression_Handler::CompressionBound(int s) {
	return LZ4_COMPRESSBOUND(s);
}
//assume dest is big enough to hold the compressed data
int RemoteDesktop::Compression_Handler::Compress(const char* source, char* dest, int inputSize, int dest_size){
	if (inputSize < 1024){
		assert(inputSize <= dest_size);
		memcpy(dest, source, inputSize);
		return -1;//no compression occurred too small to waste time trying
	}
	auto dstsize = (int*)dest;
	auto compressedsize = LZ4_compress(source, dest + sizeof(int), inputSize);
	*dstsize = inputSize;
	assert(dest_size + sizeof(int) >= compressedsize);
	return compressedsize + sizeof(int);//return new size of compressed data
}
int RemoteDesktop::Compression_Handler::Decompress(const char* source, char* dest, int compressedSize, int maxDecompressedSize){
	return LZ4_decompress_safe(source + sizeof(int), dest, compressedSize - sizeof(int), maxDecompressedSize);
}