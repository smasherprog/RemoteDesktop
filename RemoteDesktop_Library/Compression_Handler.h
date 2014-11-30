#ifndef COMPRESSION_HANDLER123_H
#define COMPRESSION_HANDLER123_H
#include <vector>


namespace RemoteDesktop{
	namespace Compression_Handler{

		int CompressionBound(int s);//worst case compression where the size grows! This can happen if you compress something that is already compressed
		int Compress(const char* source, char* dest, int inputSize);
		int Decompress(const char* source, char* dest, int compressedSize, int maxDecompressedSize);
		inline int Decompressed_Size(const char* source){ return *((int*)source); }
	};
}


#endif