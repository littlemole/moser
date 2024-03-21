#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <winmdparser/winmd_reader.h>
#include <windows.h>
#include <windows.foundation.h>
#include <rometadataresolution.h>
#include <roparameterizediid.h>

using namespace winmd::reader;

extern cache theCache;

extern std::set<std::string> typesSeen;

std::ostream& operator<<(std::ostream& oss, const char16_t rhs)
{
    return oss;
}

inline std::wstring to_wstring(const std::string& utf8)
{
    wchar_t buffer[1000];
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buffer, 1000);
    std::wstring result(buffer);
    return result;
}


inline std::string to_utf8(const std::wstring& wstr)
{
    char buffer[1000];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer, 1000, 0, 0);
    std::string result(buffer);
    return result;
}

inline std::vector<std::string> split(const std::string& s, std::string delim)
{
    std::vector<std::string> elems;

    size_t last = 0;
    size_t pos = s.find(delim, last);
    while (pos != std::string::npos)
    {
        if (pos != 0)
        {
            std::string tmp = s.substr(last, pos - last);

            if (!tmp.empty())
            {
                elems.push_back(tmp);
            }
        }
        last = pos + delim.size();
        pos = s.find(delim, last);
    }

    if (last < s.size())
    {
        elems.push_back(s.substr(last));
    }
    return elems;
}

template<class T>
std::string join(const std::vector<T>& v, const std::string& glue)
{
    if (v.empty())
        return "";

    std::ostringstream oss;

    auto it = v.begin();
    oss << *it;
    it++;
    for (it; it != v.end(); it++)
    {
        oss << glue << *it;
    }

    return oss.str();
}

inline std::string nsRoot(const std::string& ns)
{
    auto v = split(ns, ".");
    if (v.empty()) return "";
    return v[0];
}

inline std::string fullName(std::string_view ns, std::string_view n)
{
    return std::string(ns) + "." + std::string(n);
}

inline std::string guid_2_string(const GUID& guid)
{
    wchar_t* guidStr = nullptr;
    StringFromIID(guid, &guidStr);// , 256);

    std::wstring result = guidStr;
    ::CoTaskMemFree(guidStr);
    return to_utf8(result);
}

inline std::filesystem::path get_local_winmd_path()
{
    std::array<char, 260> local{};

#ifdef _WIN64
    ExpandEnvironmentStringsA("%windir%\\System32\\WinMetadata", local.data(), static_cast<uint32_t>(local.size()));
#else
    ExpandEnvironmentStringsA("%windir%\\SysNative\\WinMetadata", local.data(), static_cast<uint32_t>(local.size()));
#endif

    return local.data();
}

inline std::string find_winmd_path_for_ns(const std::string& base)
{
    //    std::filesystem::path winmd_dir = get_local_winmd_path();
    std::string wm = base + ".winmd";
    //std::cout << "? " << wm << std::endl;
    if (std::filesystem::exists(wm))
    {
        return wm;
    }
    auto v = split(base, ".");
    if (v.empty()) return "";

    v.pop_back();
    auto p = join(v, ".");
    return find_winmd_path_for_ns(p);
}


inline std::string flatten(const std::string& n)
{
    std::ostringstream oss;
    for (auto& c : n)
    {
        if (c == '.') oss << '_'; else oss << c;
    }
    return oss.str();
}

inline std::string ungen(const std::string& n)
{
    bool generic = false;
    std::ostringstream oss;
    for (auto& c : n)
    {
        if (generic == true && c == '.')
        {
            oss << "_";
            continue;
        }
        if (c == '`' || c == '<' || c == '>' || c == ',') oss << '_'; else oss << c;
        if (c == '<') generic = true;
    }
    return oss.str();
}

inline bool is_generic(std::string t)
{
    size_t pos = t.find("`");
    if (pos == std::string::npos) return false;
    return true;
}


inline std::string get_first_gen_arg(const std::string& t)
{
    size_t pos = t.find("<");
    if (pos == std::string::npos) return "";
    std::string s = t.substr(pos + 1);
    pos = s.find_first_of(">,");
    if (pos == std::string::npos) return "";

    bool isGen = s.find("`") != std::string::npos;
    if (s[pos] == ',' || isGen == false)    return s.substr(0, pos);
    return s.substr(0, pos + 1);
}


inline std::string get_second_gen_arg(const std::string& t)
{
    size_t pos = t.find("<");
    if (pos == std::string::npos) return "";
    std::string s = t.substr(pos + 1);
    pos = s.find_first_of(">,");
    if (pos == std::string::npos) return "";
    if (s[pos] == '>') return "";

    s = s.substr(pos + 1);
    pos = s.find_first_of(">,");

    return s.substr(0, pos);
}


inline bool isGenArg(std::string s)
{
    if (s[0] != 'T') return false;
    if ((s[1] < '0') || (s[1]) > '9') return false;
    return true;
}


inline bool replace(std::string& str, const std::string& from, const std::string& to) 
{
    size_t start_pos = 0;
    size_t pos = str.find(from, start_pos);
    if (pos == std::string::npos) return false;

    while (pos != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        start_pos = pos + to.length();
        pos = str.find(from, start_pos);
    }
    return true;
}


