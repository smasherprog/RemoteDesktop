#include "stdafx.h"
#include "Image.h"
#include "..\libjpeg-turbo\turbojpeg.h"
#include <memory>
#include "Timer.h"

std::vector<std::vector<char>> RemoteDesktop::INTERNAL::BufferCache;
std::mutex RemoteDesktop::INTERNAL::BufferCacheLock;

int RemoteDesktop::Image_Settings::Quality = 70;
bool RemoteDesktop::Image_Settings::GrazyScale = false;
void RemoteDesktop::Image::GetNewBuffer(){
	if (!INTERNAL::BufferCache.empty()){
		std::lock_guard<std::mutex> lock(INTERNAL::BufferCacheLock);
		if (!INTERNAL::BufferCache.empty()){
			data = std::move(INTERNAL::BufferCache.back());
			INTERNAL::BufferCache.pop_back();
		}
	}
}


void RemoteDesktop::Image::Compress(){
	if (Compressed) return;//already done

	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet because I never access these functions from different threads at the same time

	auto compfree = [](void* handle){tjDestroy(handle); };

	static std::unique_ptr<void, decltype(compfree)> _jpegCompressor;
	if (_jpegCompressor.get() == nullptr) _jpegCompressor = std::unique_ptr<void, decltype(compfree)>(tjInitCompress(), compfree);
	static std::vector<char> compressBuffer;

	auto set = Image_Settings::GrazyScale ? TJSAMP_GRAY : TJSAMP_420;

	auto maxsize = tjBufSize(Width, Height, set);
	long unsigned int _jpegSize = maxsize;

	if (compressBuffer.capacity() < maxsize) compressBuffer.reserve(maxsize + 16);

	auto t = Timer(true);

	auto ptr = (unsigned char*)compressBuffer.data();
	if (tjCompress2(_jpegCompressor.get(), (unsigned char*)data.data(), Width, 0, Height, TJPF_BGRX, &ptr, &_jpegSize, set, Image_Settings::Quality, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}
	assert(_jpegSize <= compressBuffer.capacity());
	data.resize(_jpegSize);
	memcpy(data.data(), ptr, _jpegSize);

	t.Stop();
	//DEBUG_MSG("Time Taken Compress %, size %", std::to_string(t.Elapsed_milli()), _jpegSize);

	Compressed = true;
}
void RemoteDesktop::Image::Decompress(){
	if (!Compressed) return;//already done

	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet
	auto compfree = [](void* handle){tjDestroy(handle); };
	static std::unique_ptr<void, decltype(compfree)> _jpegDecompressor;
	static std::vector<unsigned char> decompressBuffer;
	if (_jpegDecompressor.get() == nullptr) _jpegDecompressor = std::unique_ptr<void, decltype(compfree)>(tjInitDecompress(), compfree);

	size_t maxsize = Width * Height * Pixel_Stride;
	if (decompressBuffer.capacity() < maxsize) decompressBuffer.reserve(maxsize + 16);

	int jpegSubsamp = 0;
	auto outwidth = 0;
	auto outheight = 0;
	auto t = Timer(true);

	if (tjDecompressHeader2(_jpegDecompressor.get(), (unsigned char*)data.data(), data.size(), &outwidth, &outheight, &jpegSubsamp) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}

	if (tjDecompress2(_jpegDecompressor.get(), (unsigned char*)data.data(), data.size(), decompressBuffer.data(), outwidth, 0, outheight, TJPF_BGRX, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1){
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}

	t.Stop();
	//DEBUG_MSG("Time Taken Decompress %", std::to_string(t.Elapsed_milli()));
	data.resize(maxsize);//this will be the final size
	memcpy(data.data(), decompressBuffer.data(), maxsize);
	Compressed = false;
}
RemoteDesktop::Image RemoteDesktop::Image::Create_from_Compressed_Data(char* d, int size_in_bytes, int h, int w){
	Image retimg;
	retimg.data.resize(size_in_bytes);
	retimg.Height = h;
	retimg.Width = w;
	memcpy(retimg.get_Data(), d, size_in_bytes);
	retimg.Compressed = true;
	return retimg;
}

RemoteDesktop::Image RemoteDesktop::Image::Clone() const{
	auto retimg(Create_from_Compressed_Data((char*)data.data(), data.size(), Height, Width));
	retimg.Compressed = Compressed;
	return retimg;
}


RemoteDesktop::Rect RemoteDesktop::Image::Difference(Image& first, Image& second){
	assert(first.Height == second.Height);
	assert(first.Width == second.Width);
	assert(first.Pixel_Stride == second.Pixel_Stride);

	int top = -1;
	int bottom = -1;
	int left = -1;
	int right = -1;
	//all images are aligned on a 32 bit boundary so this will not overrun the arrays
	auto first_data = (int*)first.data.data();
	auto second_data = (int*)second.data.data();

	for (int y = 0; y < first.Height; y++)
	{
		for (int x = 0; x < first.Width; x += 4)
		{
			auto offset = x + (y * first.Width);
			auto la = first_data[offset] + first_data[offset + 1] + first_data[offset + 2] + first_data[offset + 3];
			auto lb = second_data[offset] + second_data[offset + 1] + second_data[offset + 2] + second_data[offset + 3];
			if (la != lb)
			{
				auto tmpx = x;
				if (top == -1)
				{
					top = y;
					if (bottom == -1)
						bottom = y + 1;
				}
				else
					bottom = y + 1;
				if (left == -1)
				{
					left = tmpx;
					if (right == -1)
						right = tmpx + 1;
				}
				else if (tmpx < left)
				{
					left = tmpx;
				}
				else if (right < tmpx)
				{
					right = tmpx + 1;
				}
			}
		}
	}


	if ((right - left <= 0) || (bottom - top <= 0))
		return Rect();
	if (left > 0)
		left -= 1;
	if (top > 0)
		top -= 1;
	if (top > 0)
		top -= 1;
	if (top > 0)
		top -= 1;
	if (right < second.Width - 1)
		right += 1;
	if (right < second.Width - 1)
		right += 1;
	if (right < second.Width - 1)
		right += 1;
	if (bottom < second.Height - 1)
		bottom += 1;

	return Rect(top, left, right - left, bottom - top);


}

RemoteDesktop::Image RemoteDesktop::Image::Copy(Image& src_img, Rect src_copy_region)
{
	auto size = src_copy_region.width * src_copy_region.height * src_img.Pixel_Stride;
	RemoteDesktop::Image retimg(src_copy_region.height, src_copy_region.width);

	auto dstend = retimg.data.data() + retimg.size_in_bytes() + 1;//this is the traditional end

	auto src = src_img.data.data();
	auto dst = retimg.data.data();

	auto srcrowstride = src_img.Width * src_img.Pixel_Stride;
	auto dstrowstride = src_copy_region.width * src_img.Pixel_Stride;
	for (int y = 0; y < src_copy_region.height; y++)
	{
		auto srcrow = (int*)(src + (srcrowstride * (y + src_copy_region.top)));
		srcrow += src_copy_region.left;
		auto dstrow = dst + (dstrowstride * y);
		assert(dstrow + dstrowstride < dstend);
		memcpy(dstrow, srcrow, dstrowstride);
	}

	return retimg;
}
void RemoteDesktop::Image::Copy(Image& src_img, int dst_left, int dst_top, int dst_stride, char* dst, int dst_height, int dst_width)
{
	auto src = src_img.data.data();
	auto jumpsrcrowstride = src_img.Width * src_img.Pixel_Stride;

	//check that the image does not overrun the array bounds
	auto hmod = dst_height - (src_img.Height + dst_top);
	if (hmod < 0) src_img.Height += hmod;
	auto wmod = dst_width - (src_img.Width + dst_left);
	if (wmod < 0) src_img.Width += wmod;

	auto dstend = dst + (dst_height* dst_width * 4) + 1;//this is the traditional end

	auto copysrcstide = src_img.Width * src_img.Pixel_Stride;

	for (int y = 0; y < src_img.Height; y++)
	{
		auto dstrow = (int*)(dst + (dst_stride * (y + dst_top)));
		dstrow += dst_left;
		auto srcrow = src + (jumpsrcrowstride * y);
		assert(((char*)dstrow) + copysrcstide < dstend);
		memcpy(dstrow, srcrow, copysrcstide);
	}
}
void RemoteDesktop::Image::Save(std::string outfile){
	assert(!Compressed);

	BITMAPINFOHEADER   bi;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Width;
	bi.biHeight = -Height;
	bi.biPlanes = 1;
	bi.biBitCount = Pixel_Stride * 8;
	bi.biCompression = BI_RGB;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	bi.biSizeImage = ((Width * bi.biBitCount + 31) / 32) * Pixel_Stride * Height;
	SaveBMP(bi, data.data(), outfile);
}
void RemoteDesktop::SaveBMP(BITMAPINFOHEADER bi, char* imgdata, std::string dst){
	BITMAPFILEHEADER   bmfHeader;
	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFileA(dst.c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = bi.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)imgdata, bi.biSizeImage, &dwBytesWritten, NULL);

	CloseHandle(hFile);

}
