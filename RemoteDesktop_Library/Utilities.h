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

template<typename... Args>
void DEBUG_MSG(const char *s, Args... args)
{
	std::string result = "";
	_INTERNAL::xsprintf(result, s, args...);
	std::cout << result << std::endl;
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


inline std::string GetFileExtention(const std::string& file){// this will only strip off everything include the . of an extention
	std::string::size_type po = file.find_last_of('.');
	if (po == std::string::npos) return "";// no extention, return an empty std::string
	return file.substr(po, file.size() - po + 1);
}
inline std::string StripFileExtention(const std::string& file){// this will only strip off everything include the . of an extention
	std::string::size_type po = file.find_last_of('.');
	if (po == std::string::npos) return file;// no extention, return the entire std::string
	return file.substr(0, po);
}
inline std::string GetPath(const std::string& file){ // this will return the path of a std::string passed to ut
	std::string::size_type po = file.find_last_of('\\');
	if (po == std::string::npos) po = file.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	if (po == std::string::npos) return "\\";// no slashes.. must be something strange.. try to return something usefull
	return file.substr(0, po + 1);
}// utility function to return only the path
inline std::string StripPath(const std::string& file){// this will only strip off the entire path, so only a filename remains
	std::string::size_type po = file.find_last_of('\\');
	if (po == std::string::npos) po = file.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	if (po == std::string::npos) return file;// no slashes.. there must be no path on this std::string
	po += 1;
	return file.substr(po, file.size() - po);// only return the filename
}

// trim from start
inline std::string &ltrim(std::string &s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace)))); return s; }
// trim from end
inline std::string &rtrim(std::string &s) { s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); return s; }
// trim from both ends
inline std::string &trim(std::string &s) { return ltrim(rtrim(s)); }

inline bool ContainsPath(const std::string& str){
	std::string::size_type po = str.find_last_of('\\');
	if (po == std::string::npos) po = str.find_last_of('/');// couldnt find it with the double slashes, try a single forward slash
	return po != std::string::npos;
}
inline bool FileExists(const std::string& f){
	struct stat fileInfo;
	return stat(f.c_str(), &fileInfo) == 0;
}

#endif