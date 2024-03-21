#include "win32/uni.h"

#ifdef _WIN32
#include <windows.h>

std::string to_string(const wchar_t* str, int nchars, long cp)
{
	int len = ::WideCharToMultiByte(cp, 0, str, nchars, 0, 0, 0, 0);
	cbuff buf(len);

	int n = ::WideCharToMultiByte(cp, 0, str, nchars, buf, len, 0, 0);
	return buf.toString(n);
}

std::string to_string(const std::wstring& in, long cp)
{
	return to_string(in.c_str(), (int)in.size(), cp);
}

std::wstring to_wstring(const char* str, int nchars, long cp)
{
	int len = ::MultiByteToWideChar(cp, 0, str, nchars, 0, 0);
	wbuff buf(len);

	::MultiByteToWideChar(cp, 0, str, nchars, buf, len);
	return buf.toString(len);
}

std::wstring to_wstring(const std::string& in, long cp)
{
	return to_wstring(in.c_str(), (int)in.size(), cp);
}

#endif 


std::wstring trim(const std::wstring& in)
{
	size_t first = in.find_first_not_of(L" \r\n\t");
	size_t last = in.find_last_not_of(L" \r\n\t");
	if ((first == std::string::npos) && (last == std::string::npos))
		return L"";
	size_t from = (first != std::string::npos) ? first : 0;
	size_t to = (last != std::string::npos) ? last - from + 1 : in.size() - from;
	return in.substr(from, to);
}

