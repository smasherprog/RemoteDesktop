#include "stdafx.h"
#include "Image.h"
#include "..\libjpeg-turbo\turbojpeg.h"
#include <memory>
#include "Timer.h"


int RemoteDesktop::Image_Settings::Quality = 70;
bool RemoteDesktop::Image_Settings::GrazyScale = false;

void RemoteDesktop::Image::Compress(){
	if (Compressed) return;//already done
	int dstoffset = 0;
	auto src = (const int*)data.data();//src data will always be rgba
	auto dst = (char*)data.data();

	for (auto y = 0; y < Height; y++){
		for (auto x = 0; x < Width; x++){
			*((int*)(dst + dstoffset)) = src[y * Width + x];
			dstoffset += 3;//advance by three bytes
		}
	}	

	Pixel_Stride = 3;
	
	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet because I never access these functions from different threads at the same time

	static std::shared_ptr<void> _jpegCompressor;
	if (_jpegCompressor.get() == nullptr) _jpegCompressor = std::shared_ptr<void>(tjInitCompress(), [](void* handle){tjDestroy(handle); });
	static std::vector<char> compressBuffer;

	auto maxsize = Width * Height * 4;
	long unsigned int _jpegSize = maxsize;
	
	if (compressBuffer.capacity() < maxsize) compressBuffer.reserve(maxsize);

	auto t = Timer(true);
	
	auto ptr = (unsigned char*)compressBuffer.data();
	auto set = Image_Settings::GrazyScale ? TJSAMP_GRAY : TJSAMP_420;
	if (tjCompress2(_jpegCompressor.get(), (unsigned char*)data.data(), Width, 0, Height, TJPF_BGR, &ptr, &_jpegSize, set, Image_Settings::Quality, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}
	assert(_jpegSize <= compressBuffer.capacity());
	data.resize(_jpegSize);
	memcpy(data.data(), ptr, _jpegSize);

	t.Stop();
	//DEBUG_MSG("Time Taken Compress %", std::to_string(t.Elapsed_milli()));
	Compressed = true;
}
void RemoteDesktop::Image::Decompress(){
	if (!Compressed) return;//already done

	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet
	static std::shared_ptr<void> _jpegDecompressor;
	static std::vector<unsigned char> decompressBuffer;
	if (_jpegDecompressor.get() == nullptr) _jpegDecompressor = std::shared_ptr<void>(tjInitDecompress(), [](void* handle){tjDestroy(handle); });
	
	auto maxsize = Width * Height * 4;
	if (decompressBuffer.capacity() < maxsize) decompressBuffer.reserve(maxsize);

	int jpegSubsamp = 0;
	auto outwidth = 0;
	auto outheight = 0;
	auto t = Timer(true);

	if (tjDecompressHeader2(_jpegDecompressor.get(), (unsigned char*)data.data(), data.size(), &outwidth, &outheight, &jpegSubsamp) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}


	if (tjDecompress2(_jpegDecompressor.get(), (unsigned char*)data.data(), data.size(), decompressBuffer.data(), outwidth, 0, outheight, TJPF_BGR, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1){
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}

	t.Stop();
	//DEBUG_MSG("Time Taken Decompress %", std::to_string(t.Elapsed_milli()));
	Pixel_Stride = 4;
	data.resize(outwidth*outheight*Pixel_Stride);//this will be the final size
	int srcoffset = 0;
	auto src = (const char*)decompressBuffer.data();//src data will always be rgb
	auto dst = (int*)data.data();
	union rgba {
		int data;
		unsigned char cdata[4];
	};
	
	for (auto y = 0; y < outheight; y++){
		for (auto x = 0; x < outwidth; x++){
			rgba d;
			d.data = *((int*)(src + srcoffset));
			d.cdata[3] = 255;
			dst[y * outwidth + x] = d.data;
			srcoffset += 3;//advance by three bytes
		}
	}
	Compressed = false;
}
RemoteDesktop::Image RemoteDesktop::Image::Create_from_Compressed_Data(char* d, int size_in_bytes, int px_stride, int h, int w){
	Image retimg;
	retimg.data.resize(size_in_bytes);
	retimg.Pixel_Stride = px_stride;
	retimg.Height = h;
	retimg.Width = w;
	memcpy(retimg.get_Data(), d, size_in_bytes);
	retimg.Compressed = true;
	return retimg;
}

RemoteDesktop::Image RemoteDesktop::Image::Clone() const{
	auto retimg(Create_from_Compressed_Data((char*)data.data(), data.size(), Pixel_Stride, Height, Width));
	retimg.Compressed = Compressed;
		return retimg;
}


RemoteDesktop::Rect RemoteDesktop::Image::Difference(Image first, Image second){
	int top = -1;
	int bottom = -1;
	int left = -1;
	int right = -1;


	auto even = (first.Width % first.Pixel_Stride);
	auto totalwidth = first.Width - even; //subtract any extra bits to ensure I dont go over the array bounds
	auto stide = (first.Width * first.Pixel_Stride);

	for (int y = 0; y < first.Height; y++)
	{
		auto linea = (int*)(first.data.data() + (stide *y));
		auto lineb = (int*)(second.data.data() + (stide *y));

		for (int x = 0; x < totalwidth; x += 4)
		{

			auto la = linea[x] + linea[x + 1] + linea[x + 2] + linea[x + 3];
			auto lb = lineb[x] + lineb[x + 1] + lineb[x + 2] + lineb[x + 3];
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

RemoteDesktop::Image RemoteDesktop::Image::Copy(Image src_img, Rect src_copy_region)
{
	auto size = src_copy_region.width * src_copy_region.height * src_img.Pixel_Stride;
	RemoteDesktop::Image retimg(src_img.Pixel_Stride, src_copy_region.height, src_copy_region.width);
	
	auto src = src_img.data.data();
	auto dst = retimg.data.data();

	auto srcrowstride = src_img.Width * src_img.Pixel_Stride;
	auto dstrowstride = src_copy_region.width * src_img.Pixel_Stride;
	for (int y = 0; y < src_copy_region.height; y++)
	{
		auto srcrow = (int*)(src + (srcrowstride * (y + src_copy_region.top)));
		srcrow += src_copy_region.left;
		auto dstrow = dst + (dstrowstride * y);
		memcpy(dstrow, srcrow, dstrowstride);
	}
	return retimg;
}
void RemoteDesktop::Image::Copy(Image src_img, int dst_left, int dst_top, int dst_stride, char* dst, int dst_height, int dst_width)
{
	auto src = src_img.data.data();
	auto jumpsrcrowstride = src_img.Width * src_img.Pixel_Stride;

	//check that the image does not overrun the array bounds
	auto hmod = dst_height - (src_img.Height + dst_top);
	if (hmod < 0) src_img.Height += hmod;
	auto wmod = dst_width - (src_img.Width + dst_left);
	if (wmod < 0) src_img.Width += wmod;

	auto copysrcstide = src_img.Width * src_img.Pixel_Stride;

	for (int y = 0; y < src_img.Height; y++)
	{
		auto dstrow = (int*)(dst + (dst_stride * (y + dst_top)));
		dstrow += dst_left;
		auto srcrow = src + (jumpsrcrowstride * y);
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
	bi.biBitCount = Pixel_Stride*8;
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
