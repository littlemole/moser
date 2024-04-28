#include "pch.h"
#include "common.h"
#include <string>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>

#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>


#ifndef _WIN32
#include <unistd.h> // getcwd
#include <linux/limits.h>
#include <link.h>
#include <dlfcn.h>
#else
#include <windows.h>
#include <direct.h>
#include "win32/uni.h"
#endif

long get_mtime(const char *path) 
{
    struct stat attr;
    stat(path, &attr);
#ifdef _WIN32
    return (long)attr.st_mtime;
#else    
    return attr.st_mtim.tv_sec;
#endif
}

std::string trim(const std::string& input)
{
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = input.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) end = input.size() - 1;

    return input.substr(start, end - start + 1);
}


std::string slurp(const char* path) 
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) 
    {
        return "";
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) 
    {
        return "";
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) 
    {
        return "";
    }

    buffer[bytesRead] = '\0';

    std::string result(buffer,bytesRead);
    free(buffer);
    fclose(file);

    return result;
}

std::string unescape(const std::string& str)
{
    std::ostringstream oss;

    const size_t len = str.size();
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] == '\\')
        {
            if (i < len - 1)
            {
                const char c = str[i + 1];
                switch (c)
                {
                case 'n': {
                    oss << "\n";
                    i++;
                    continue;
                }
                case 'r': {
                    oss << "\r";
                    i++;
                    continue;
                }
                case 't': {
                    oss << "\t";
                    i++;
                    continue;
                }
                case '\\': {
                    oss << "\\";
                    i++;
                    continue;
                }
                case '"': {
                    oss << "\"";
                    i++;
                    continue;
                }
                case '0': {
                    static const char zero = '\0';
                    oss.write(&zero, 1);
                    i++;
                    continue;
                }
                }
            }
        }
        oss << str[i];
    }
    return oss.str();
}

std::string join(std::vector<std::string>& items, const std::string& glue)
{
    std::ostringstream oss;
    auto it = items.begin();
    if (!items.empty())
    {
        oss << (*it);
        it++;
    }
    while (it != items.end())
    {
        oss << glue << (*it);
        it++;
    }
    return oss.str();
}

std::vector<std::string> split(const std::string& s, std::string delim)
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

std::string fixNewline(const std::string& s)
{
    std::ostringstream oss;
    for (size_t i = 0; i < s.size(); i++)
    {
        const char it = s[i];
        oss.write(&it, 1);
        if (it == 13)
        {
            if (i < s.size() - 1 && s[i + 1] == 10) continue;
            static const char nl = '\n';
            oss.write( &nl, 1);
        }
    }
    return oss.str();
}


std::string toDos(const std::string& s)
{
    static const char* nl = "\r\n";

    std::ostringstream oss;
    for (size_t i = 0; i < s.size(); i++)
    {
        const char it = s[i];
        switch (it)
        {
            case '\r' :
            {
                if ( (i < s.size() - 1) && (s[i+1] == '\n'))
                {
                    oss.write(nl, 2);
                    i++;
                    continue;
                }
                oss.write(nl, 2);
                break;
            }
            case '\n':
            {
                oss.write(nl, 2);
                continue;
                break;
            }
            default :
            {
                oss.write(&it, 1);
            }
        }
    }
    return oss.str();
}

std::string toUnix(const std::string& s)
{
    static const char* nl = "\n";

    std::ostringstream oss;
    for (size_t i = 0; i < s.size(); i++)
    {
        const char it = s[i];
        switch (it)
        {
            case '\r':
            {
                if ( (i < s.size() - 1) && (s[i+1] == '\n') )
                {
                    oss.write(nl, 1);
                    i++;
                    continue;
                }
                oss.write(nl, 1);
                break;
            }
            default:
            {
                oss.write(&it, 1);
            }
        }
    }
    return oss.str();
}

void flush(const std::string& fp, const std::string& content)
{
    std::ofstream ofs;
    ofs.open(fp, std::ios::binary);
    if (!ofs)
    {
        return;
    }
    ofs.write(content.data(), content.size());

    ofs.close();
}


static char nibble_decode(char nibble)
{
    const char byte_map[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };
    return byte_map[(int)nibble];
}

std::string toHex(const std::string& input)
{
    unsigned char* bytes = (unsigned char*)input.c_str();

    std::ostringstream oss;
    for (size_t i = 0; i < input.size(); i++)
    {
        char c1 = nibble_decode(bytes[i] >> 4);
        char c2 = nibble_decode(bytes[i] & 0x0f);
        oss.write(&c1, 1);
        oss.write(&c2, 1);
    }
    return oss.str();
}


static int HexCharToInt(char ch)
{
    if (ch >= '0' && ch <= '9')
        return (ch - '0');
    else if (ch >= 'a' && ch <= 'f')
        return (ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F')
        return (ch - 'A' + 10);
    else
        return 0;
}


static int HexByteToInt(const char* hex)
{
    return HexCharToInt(hex[0]) * 16
        + HexCharToInt(hex[1]);
}


std::string fromHex(const std::string& hex)
{
    std::ostringstream oss;

    const char* p = hex.c_str();
    while (*p)
    {
        char i = (char) HexByteToInt((char*)p);
        oss.write(&i, 1);
        p += 2;
    }

    return oss.str();
}

#ifdef _WIN32
inline HMODULE get_self_module()
{
    HMODULE hm = NULL;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&get_self_module, &hm) == 0)
    {
        int ret = GetLastError();
        fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
        exit(1);
    }
    return hm;
}
#endif

std::string path_to_self()
{
#ifdef _WIN32
    HMODULE hm = get_self_module();

    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(hm, path, MAX_PATH) == 0)
    {
        int ret = GetLastError();
        fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
        exit(1);
    }
    return to_string(std::wstring(path));
#else

    char buf[PATH_MAX + 1];
    std::ostringstream oss;
    oss << "/proc/" << getpid() << "/exe";
    if (readlink(oss.str().c_str(), buf, sizeof(buf) - 1) == -1)
        return "XXX";
    std::string str(buf);
    return str;
#endif
}

std::string current_working_directory()
{
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    ::GetCurrentDirectoryW(MAX_PATH, buf);
    return to_string(std::wstring(buf));
#else
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) 
    {
        std::string result(cwd);
        return result;
    }
    return "";
#endif
}


extern const char* sep;

std::string path_to_self_directory(const char* postfix)
{
    std::string self = path_to_self();

    std::size_t pos = self.find_last_of(sep);
    if (pos == std::wstring::npos)
    {
        return "";
    }

    if (postfix)
    {
        std::ostringstream oss;
        oss << self.substr(0, pos) << sep << postfix;
        return oss.str();
    }

    return self.substr(0, pos);
}

