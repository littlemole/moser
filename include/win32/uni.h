#ifndef moc_uni_h
#define moc_uni_h

#include <string>
#include <vector>
#include <cstring>

#ifdef _WIN32

/*
	MOSER support for Win32 16 bit Unicode strings
*/

template<class T>
class buffer
{
public:

	buffer()
	{}

	buffer(size_t n)
		: buf(n + 1, 0)
	{
		buf[n] = 0;
	}

	buffer(size_t n, int value)
		: buf(n + 1, 0)
	{
		::memset(&buf[0], value, (n) * sizeof(T));
		buf[n] = 0;
	}

	buffer(const buffer& rhs)
		: buf(rhs.buf)
	{}

	buffer(buffer&& rhs)
		: buf(std::move(rhs.buf))
	{
		rhs.buf.clear();
	}

	buffer& operator=(const buffer& rhs)
	{
		if (&rhs == this)
			return *this;
		buf = rhs;
		return *this;
	}

	buffer& operator=(buffer&& rhs)
	{
		if (&rhs == this)
			return *this;
		buf = std::move(rhs.buf);
		rhs.buf.clear();
		return *this;
	}

	void alloc(size_t n)
	{
		buf = std::vector<T>(n + 1);
		buf[n] = 0;
	}

	T* operator&()
	{
		return &buf[0];
	}

	operator const T* () const
	{
		return &buf[0];
	}

	operator T* ()
	{
		return &buf[0];
	}


	size_t size() const
	{
		return buf.size() - 1;
	}

	std::basic_string<T> toString(size_t len = std::basic_string<T>::npos) const
	{
		if (len != std::basic_string<T>::npos)
		{
			return std::basic_string<T>(&buf[0], len);
		}
		return std::basic_string<T>(&buf[0]);
	}

private:
	std::vector<T> buf;
};

typedef buffer<char> cbuff;
typedef buffer<wchar_t> wbuff;

std::string to_string(const wchar_t* str, int nchars = -1, long cp = 65001); //CP_UTF8
std::string to_string(const std::wstring& in, long cp = 65001);
std::wstring to_wstring(const char* str, int nchars = -1, long cp = 65001);
std::wstring to_wstring(const std::string& in, long cp = 65001);

#endif

std::wstring trim(const std::wstring& in);

#endif
