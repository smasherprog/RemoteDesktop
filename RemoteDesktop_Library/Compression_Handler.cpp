#include "stdafx.h"
#include "Compression_Handler.h"
#include "lz4.h"

int RemoteDesktop::Compression_Handler::CompressionBound(int s) {
	return LZ4_COMPRESSBOUND(s);
}
//assume dest is big enough to hold the compressed data
int RemoteDesktop::Compression_Handler::Compress(const char* source, char* dest, int inputSize){
	if (inputSize < 1024){
		memcpy(dest, source, inputSize);
		return -1;//no compression occured
	}

	auto t1 = Timer(true);

	auto dstsize = (int*)dest;
	auto compressedsize = LZ4_compress(source, dest + sizeof(int), inputSize);
	t1.Stop();
	DEBUG_MSG("2) Compressedsize = %, time %", compressedsize, t1.Elapsed_milli());

	*dstsize = inputSize;
	return compressedsize + sizeof(int);//return new size of compressed data
}
int RemoteDesktop::Compression_Handler::Decompress(const char* source, char* dest, int compressedSize, int maxDecompressedSize){
	return LZ4_decompress_safe(source + sizeof(int), dest, compressedSize - sizeof(int), maxDecompressedSize);
}