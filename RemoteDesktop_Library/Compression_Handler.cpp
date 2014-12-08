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
		memcpy(dest, source, inputSize);
		return -1;//no compression occured
	}

	auto dstsize = (int*)dest;
	auto compressedsize = 0;
	//Tests Below.. a compression value of 4 gives the best bang for the buck when normal browsing.. For extreme data cases, the standard compression works be
	//auto t = Timer(true);
	// = LZ4_compressHC(source, dest + sizeof(int), inputSize);

	//t.Stop();
	//DEBUG_MSG("1) Time %, size %", t.Elapsed_milli(), firstcompress);

	//for (auto i = 0; i < 7; i++){
	//	t.Start();
	//	compressedsize = LZ4_compressHC2_limitedOutput(source, dest + sizeof(int), inputSize, dest_size, i);
	//	t.Stop();
	//	DEBUG_MSG("EX) Time %, size %, Setting %", t.Elapsed_milli(), firstcompress, i);
	//}

	//t.Start();
	//compressedsize = LZ4_compress(source, dest + sizeof(int), inputSize);
	//t.Stop();
	//DEBUG_MSG("3) Time %, size %", t.Elapsed_milli(), firstcompress);
	//
	//compressedsize = LZ4_compressHC2_limitedOutput(source, dest + sizeof(int), inputSize, dest_size, 4);	

	compressedsize = LZ4_compress(source, dest + sizeof(int), inputSize);
	*dstsize = inputSize;
	return compressedsize + sizeof(int);//return new size of compressed data
}
int RemoteDesktop::Compression_Handler::Decompress(const char* source, char* dest, int compressedSize, int maxDecompressedSize){
	return LZ4_decompress_safe(source + sizeof(int), dest, compressedSize - sizeof(int), maxDecompressedSize);
}