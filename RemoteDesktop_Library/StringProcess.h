#ifndef STRINGPROCESS_H
#define STRINGPROCESS_H
#include <string>
#include <atlcomtime.h>
#include <vector>

std::wstring Trim(const std::wstring &source, const std::wstring &targets);
bool PrepareString(wchar_t *dest, size_t *size, const std::wstring &src);
std::wstring ReplaceString(const std::wstring &srcStr, const std::wstring &oldStr, const std::wstring &newStr);
int StringToInteger(const std::wstring &number);
std::wstring LowerString(const std::wstring &text);
std::wstring UpperString(const std::wstring &text);
std::wstring GetAnchorText(const std::wstring &anchor);
std::wstring GetAnchorLink(const std::wstring &anchor);
bool SeparateString(const std::wstring &content, const std::wstring &delimiter, std::vector<std::wstring> &result);
std::wstring URLEncoding(const std::wstring &keyword, bool convertToUTF8 = true);
unsigned int GetSeparateKeywordMatchGrade(const std::wstring &source, const std::wstring &keyword);

unsigned int GetKeywordMatchGrade(const std::wstring &source, const std::wstring & keyword);
std::wstring GetDateString(const COleDateTime &time, const std::wstring &separator = L"-", bool align = true);
std::wstring GetDateString(int dayOffset, const std::wstring &separator = L"-", bool align = true);
std::wstring GetTimeString(const COleDateTime &time, const std::wstring &separator = L":", bool align = true);
std::wstring MD5(const std::wstring &text);

std::wstring FilterFileName(const std::wstring &name);
std::wstring GetMagic(unsigned int length);
std::wstring GetHost(const std::wstring &url);

std::wstring GetValidFileName(const std::wstring &fileName);

#endif // STRINGPROCESS_H
