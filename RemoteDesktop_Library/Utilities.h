#ifndef UTILITIES_DEBUG_H
#define UTILITIES_DEBUG_H

#include <iostream>
#include <assert.h>
#include <string>
#include <sys/stat.h>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>

inline int roundUp(int numToRound, int multiple)//only for multiples of 2
{
	return (numToRound + multiple - 1) & ~(multiple - 1);
}

template<typename... Args>
void DEBUG_MSG(const char *s, Args... args)
{

	std::string result = "";
	_INTERNAL::xsprintf(result, s, args...);
	OutputDebugStringA(result.c_str());
	OutputDebugStringA("\n");
}

namespace _INTERNAL{

	inline void xsprintf(std::string& result, const char *s)
	{
		while (*s) {
			if (*s == '%') {
				if (*(s + 1) == '%') {
					++s;
				}
				else {
					throw std::runtime_error("invalid format string: missing arguments");
				}
			}
			result += *s++;
		}
	}

	template<typename T, typename... Args>
	void xsprintf(std::string& result, const char *s, T value, Args... args)
	{
		while (*s) {
			if (*s == '%') {
				if (*(s + 1) == '%') {
					++s;
				}
				else {
					std::stringstream stream;
					stream << value;
					result += stream.str();
					xsprintf(result, s + 1, args...); // call even when *s == 0 to detect extra arguments
					return;
				}
			}
			result += *s++;
		}
		throw std::logic_error("extra arguments provided to printf");
	}
}
// templated version of my_equal so it could work with both char and wchar_t
template<typename charT>
struct my_equal {
	my_equal(const std::locale& loc) : loc_(loc) {}
	bool operator()(charT ch1, charT ch2) {
		return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
	}
private:
	const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
int find_substr(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
	typename T::const_iterator it = std::search(str1.begin(), str1.end(),
		str2.begin(), str2.end(), my_equal<typename T::value_type>(loc));
	if (it != str1.end()) return it - str1.begin();
	else return -1; // not found
}
inline std::wstring FormatBytes(long long bytes)
{
	const auto scale = 1024.0;
	static std::wstring orders[] =  { L"GB/s", L"MB/s", L"KB/s", L"Bytes/s" };

	auto max = (long long)pow(scale, 3.0);
	for(auto& order : orders)
	{
		if (bytes > max){
			std::wstringstream str;
			str << std::fixed << std::setprecision(2) << ((float)bytes / (float)max);
			return str.str() + order;
		}
			
		//	return string.Format("{0:##.##} {1}", Decimal.Divide(bytes, max), order);
		max /= scale;
	}
	return L"0 Bytes";
}


// no need to check if(x), in c++, delete always checks that before deleting
#ifndef RELEASECOM
#define RELEASECOM(x) { if(x) x->Release(); x=0; }
#endif
#ifndef DELETE_ARRAY
#define DELETE_ARRAY(x) { delete[] (x);  x=nullptr; } 
#endif
// not really safe because delete is already safe, but saves me an extra line of code :P
#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { delete (x);  x=nullptr; } 
#endif

#ifdef  NDEBUG
#define assert2(_Expression, _Msg) ((void)0)
#else 
#define assert2(_Expression, _Msg) (void)( (!!(_Expression)) || (_wassert(_Msg, _CRT_WIDE(__FILE__), __LINE__), 0) )
#endif

inline std::ifstream::pos_type filesize(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}
inline std::vector<std::wstring> split(const std::wstring& o, const wchar_t d){
	std::wstringstream f(o);
	std::wstring s;
	std::vector<std::wstring> strings;
	while (getline(f, s, d)) {
		strings.push_back(s);
	}
	return strings;
}
inline std::vector<std::string> split(const std::string& o, const char d){
	std::stringstream f(o);
	std::string s;
	std::vector<std::string> strings;
	while (getline(f, s, d)) {
		strings.push_back(s);
	}
	return strings;
}
template<class T> T GetFileExtention(const T& file){// this will only strip off everything include the . of an extention
	T::size_type po = file.find_last_of('.');
	if (po == T::npos) return "";// no extention, return an empty std::string
	return file.substr(po, file.size() - po + 1);
}
template<class T>T StripFileExtention(const T& file){// this will only strip off everything include the . of an extention
	T::size_type po = file.find_last_of('.');
	if (po == T::npos) return file;// no extention, return the entire std::string
	return file.substr(0, po);
}
template<class T> T GetPath(const T& file){ // this will return the path of a std::string passed to ut
	T::size_type po = file.find_last_of('\\');
	if (po == T::npos) po = file.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	if (po == T::npos) return "\\";// no slashes.. must be something strange.. try to return something usefull
	return file.substr(0, po + 1);
}// utility function to return only the path
template<class T>T StripPath(const T& file){// this will only strip off the entire path, so only a filename remains
	T::size_type po = file.find_last_of('\\');
	if (po == T::npos) po = file.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	if (po == T::npos) return file;// no slashes.. there must be no path on this std::string
	po += 1;
	return file.substr(po, file.size() - po);// only return the filename
}

// trim from start
template<class T>T &ltrim(T &s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace)))); return s; }
// trim from end
template<class T>T &rtrim(T &s) { s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); return s; }
// trim from both ends
template<class T>T &trim(T &s) { return ltrim(rtrim(s)); }

inline bool ContainsPath(const std::string& str){
	std::string::size_type po = str.find_last_of('\\');
	if (po == std::string::npos) po = str.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	return po != std::string::npos;
}
inline bool FileExists(const std::string& f){
	struct stat fileInfo;
	return stat(f.c_str(), &fileInfo) == 0;
}
inline bool IsFile(const std::string& f){
	struct stat info;
	if (stat(f.c_str(), &info) != 0) return false;
	else if (info.st_mode & S_IFDIR)  return false;
	else return true;
}


class DynamicFnBase {
public:
	DynamicFnBase(const char* dllName, const char* fnName) : dllHandle(0), fnPtr(0) {
		dllHandle = LoadLibraryA(dllName);
		if (!dllHandle) {
			return;
		}
		fnPtr = (void *)GetProcAddress(dllHandle, fnName);
	}
	~DynamicFnBase(){
		if (dllHandle)
			FreeLibrary(dllHandle);
	}
	bool isValid() const { return fnPtr != 0; }
protected:
	void* fnPtr;
	HMODULE dllHandle;
};

template<typename T> class DynamicFn : public DynamicFnBase {
public:
	DynamicFn(const char* dllName, const char* fnName) : DynamicFnBase(dllName, fnName) {}
	T operator *() const { return (T)fnPtr; };
};

inline int char2int(char input)
{
	if (input >= '0' && input <= '9')
		return input - '0';
	if (input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	if (input >= 'a' && input <= 'f')
		return input - 'a' + 10;
	return 0;
}
// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
inline void hex2bin(const char* src, char* target)
{
	while (*src && src[1])
	{
		*(target++) = char2int(*src) * 16 + char2int(src[1]);
		src += 2;
	}
}
inline std::string ws2s(const std::wstring& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	char* buf = new char[len];
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
	std::string r(buf);
	delete[] buf;
	return r;
}

#endif