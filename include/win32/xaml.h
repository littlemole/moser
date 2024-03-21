
#ifndef moc_xaml_object_h
#define moc_xaml_object_h

#ifdef _WIN32
#include <map>
#include <windows.h>
#include <roapi.h>
#include <hstring.h>
#include <windows.foundation.h>

#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <string>
#include <functional>

/*
    Helper for WinRT / XAML code to construct WinRT HSTRINGs
*/

class hstr
{
public:

    hstr()
    {}

    hstr(std::wstring s)
    {
        ::WindowsCreateString(s.c_str(), (UINT32) s.size(), &hstr_);
    }

    hstr(HSTRING rhs)
    {
        ::WindowsDuplicateString(rhs, &hstr_);
    }

    hstr(const hstr& rhs)
    {
        ::WindowsDuplicateString( *rhs, &hstr_);
    }

    hstr( hstr&& rhs)
    {
        hstr_ = rhs.hstr_;
        rhs.hstr_ = nullptr;
    }

    hstr& operator=(const hstr& rhs)
    {
        if (hstr_)
        {
            ::WindowsDeleteString(hstr_);
            hstr_ = nullptr;
        }
        ::WindowsDuplicateString(*rhs, &hstr_);
        return *this;
    }

    hstr& operator=( hstr&& rhs)
    {
        if (hstr_)
        {
            ::WindowsDeleteString(hstr_);
            hstr_ = nullptr;
        }
        hstr_ = rhs.hstr_;
        rhs.hstr_ = nullptr;
        return *this;
    }

    const HSTRING& operator*() const
    {
        return hstr_;
    }


    HSTRING* operator&()
    {
        return &hstr_;
    }

    ~hstr()
    {
        if (hstr_)
        {
            ::WindowsDeleteString(hstr_);
            hstr_ = nullptr;
        }
    }

    std::wstring str() const
    {
        if (!hstr_) return L"";

        UINT32 size = 0;
        const wchar_t* wc = ::WindowsGetStringRawBuffer(hstr_, &size);
        std::wstring result(wc, size);
        return result;
    }


    const wchar_t* c_str() const
    {
        if (!hstr_) return L"";

        UINT32 size = 0;
        return ::WindowsGetStringRawBuffer(hstr_, &size);
    }

    UINT32 length() const
    {
        if (!hstr_) return 0;
        return ::WindowsGetStringLen(hstr_);
    }

    bool empty()
    {
        return length() == 0;
    }

private:
    HSTRING hstr_ = nullptr;
};

#endif
#endif