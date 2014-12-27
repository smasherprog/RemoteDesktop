#include "stdafx.h"
#include "Image.h"
#include "..\libjpeg-turbo\turbojpeg.h"
#include <memory>
#include "Timer.h"


int RemoteDesktop::Image_Settings::Quality = 70;
bool RemoteDesktop::Image_Settings::GrazyScale = false;

void RemoteDesktop::Image::Compress(){
	if (stride == 3) return;//already done
	int dstoffset = 0;
	auto src = (const int*)data;//src data will always be rgba
	auto dst = (char*)data;

	for (auto y = 0; y < height; y++){
		for (auto x = 0; x < width; x++){
			*((int*)(dst + dstoffset)) = src[y * width + x];
			dstoffset += 3;//advance by three bytes
		}
	}	

	assert(data + dstoffset < data + size_in_bytes);//ensure I did not stomp over memory
	stride = 3;
	size_in_bytes = height*width*stride;
	

	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet because I never access these functions from different threads at the same time

	static std::shared_ptr<void> _jpegCompressor;
	if (_jpegCompressor.get() == nullptr) _jpegCompressor = std::shared_ptr<void>(tjInitCompress(), [](void* handle){tjDestroy(handle); });
	static std::vector<char> compressBuffer;

	auto maxsize = width * height * 4;
	long unsigned int _jpegSize = maxsize;
	
	if (compressBuffer.capacity() < maxsize) compressBuffer.reserve(maxsize);

	auto t = Timer(true);
	
	auto ptr = (unsigned char*)compressBuffer.data();
	auto set = Image_Settings::GrazyScale ? TJSAMP_GRAY : TJSAMP_420;
	if (tjCompress2(_jpegCompressor.get(), data, width, 0, height, TJPF_BGR, &ptr, &_jpegSize, set, Image_Settings::Quality, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}
	assert(_jpegSize <= compressBuffer.capacity());
	size_in_bytes = _jpegSize;
	memcpy(data, ptr, size_in_bytes);

	t.Stop();
	//DEBUG_MSG("Time Taken Compress %", std::to_string(t.Elapsed_milli()));

}
void RemoteDesktop::Image::Decompress(std::vector<int>& buffer){
	if (stride == 4) return;//already done

	buffer.reserve(width*height);
	
	//I Kind of cheat below by using static variables. . .  This means the compress and decompress functions are NOT THREAD SAFE, but this isnt a problem yet
	static std::shared_ptr<void> _jpegDecompressor;
	static std::vector<unsigned char> decompressBuffer;
	if (_jpegDecompressor.get() == nullptr) _jpegDecompressor = std::shared_ptr<void>(tjInitDecompress(), [](void* handle){tjDestroy(handle); });
	
	auto maxsize = width * height * 4;
	if (decompressBuffer.capacity() < maxsize) decompressBuffer.reserve(maxsize);

	int jpegSubsamp = 0;
	auto width = 0;
	auto height = 0;
	auto t = Timer(true);

	if (tjDecompressHeader2(_jpegDecompressor.get(), data, size_in_bytes, &width, &height, &jpegSubsamp) == -1) {
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}


	if (tjDecompress2(_jpegDecompressor.get(), data, size_in_bytes, decompressBuffer.data(), width, 0, height, TJPF_BGR, TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1){
		DEBUG_MSG("Err msg %", tjGetErrorStr());
	}

	t.Stop();
	//DEBUG_MSG("Time Taken Decompress %", std::to_string(t.Elapsed_milli()));


	int srcoffset = 0;
	auto src = (const char*)decompressBuffer.data();//src data will always be rgb
	auto dst = buffer.data();
	union rgba {
		int data;
		unsigned char cdata[4];
	};
	
	for (auto y = 0; y < height; y++){
		for (auto x = 0; x < width; x++){
			rgba d;
			d.data = *((int*)(src + srcoffset));
			d.cdata[3] = 255;
			dst[y * width + x] = d.data;
			srcoffset += 3;//advance by three bytes
		}
	}
	data = (unsigned char*)buffer.data();
	stride = 4;
	size_in_bytes = height*width*stride;
}

RemoteDesktop::Image RemoteDesktop::Image::Clone(std::vector<char>& buffer) const{
	buffer.reserve(size_in_bytes);
	memcpy(buffer.data(), data, size_in_bytes);
	return Image((unsigned char*)buffer.data(), size_in_bytes, height, width);
}

RemoteDesktop::Rect RemoteDesktop::Image::Difference(Image first, Image second){
	int top = -1;
	int bottom = -1;
	int left = -1;
	int right = -1;


	auto even = (first.width % 4);
	auto totalwidth = first.width - even; //subtract any extra bits to ensure I dont go over the array bounds
	auto stide = (first.width * 4);

	for (int y = 0; y < first.height; y++)
	{
		auto linea = (int*)(first.data + (stide *y));
		auto lineb = (int*)(second.data + (stide *y));

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
	if (right < second.width - 1)
		right += 1;
	if (right < second.width - 1)
		right += 1;
	if (right < second.width - 1)
		right += 1;
	if (bottom < second.height - 1)
		bottom += 1;

	return Rect(top, left, right - left, bottom - top);


}

RemoteDesktop::Image RemoteDesktop::Image::Copy(Image src_img, Rect src_copy_region, std::vector<unsigned char>& buffer)
{
	auto size = src_copy_region.width * src_copy_region.height * 4;
	buffer.reserve(size);
	auto src = src_img.data;
	auto dst = buffer.data();

	auto srcrowstride = src_img.width * 4;
	auto dstrowstride = src_copy_region.width * 4;
	for (int y = 0; y < src_copy_region.height; y++)
	{
		auto srcrow = (int*)(src + (srcrowstride * (y + src_copy_region.top)));
		srcrow += src_copy_region.left;
		auto dstrow = dst + (dstrowstride * y);
		memcpy(dstrow, srcrow, dstrowstride);
	}
	return Image(buffer.data(), size, src_copy_region.height, src_copy_region.width);
}
void RemoteDesktop::Image::Copy(Image src_img, int dst_left, int dst_top, int dst_stride, unsigned char* dst, int dst_height, int dst_width)
{
	auto src = src_img.data;
	auto jumpsrcrowstride = src_img.width * 4;

	//check that the image does not overrun the array bounds
	auto hmod = dst_height - (src_img.height + dst_top);
	if (hmod < 0) src_img.height += hmod;
	auto wmod = dst_width - (src_img.width + dst_left);
	if (wmod < 0) src_img.width += wmod;

	auto copysrcstide = src_img.width * 4;

	for (int y = 0; y < src_img.height; y++)
	{
		auto dstrow = (int*)(dst + (dst_stride * (y + dst_top)));
		dstrow += dst_left;
		auto srcrow = src + (jumpsrcrowstride * y);
		memcpy(dstrow, srcrow, copysrcstide);
	}
}