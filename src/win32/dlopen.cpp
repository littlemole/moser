#include "pch.h"
#include "win32/dlopen.h"
#include "win32/uni.h"

#ifdef _WIN32

#include <windows.h>

void* dlopen(const char* file,   /** DLL filename (UTF-8). */ int mode            /** mode flags (ignored). */)
{
    std::wstring unicodeFilename;
    UINT errorMode;
    void* handle;

    UNREFERENCED_PARAMETER(mode);

    if (file == NULL)
        return (void*)::GetModuleHandle(nullptr);

    unicodeFilename = to_wstring(file);

    if (unicodeFilename.empty()) 
    {
        return nullptr;
    }

    errorMode = GetErrorMode();

    /* Have LoadLibrary return NULL on failure; prevent GUI error message. */
    ::SetErrorMode(errorMode | SEM_FAILCRITICALERRORS);

    handle = (void*)::LoadLibraryW(unicodeFilename.c_str());

    if (handle == nullptr)
        return nullptr;

    ::SetErrorMode(errorMode);

    return handle;
}

int dlclose(void* handle        /** Handle from dlopen(). */)
{
    int rc = 0;

    if (handle != (void*)::GetModuleHandle(nullptr))
    {
        bool b = (bool) ::FreeLibrary((HMODULE)handle);
        rc = !b;
    }

    return rc;
}

/**
 * Look up symbol exported by DLL.
 */

void* dlsym(void* handle,       /** Handle from dlopen(). */const char* name    /** Name of exported symbol (ASCII). */)
{
    void* address = (void*)::GetProcAddress((HMODULE)handle, name);

    return address;
}

const char* dlerror()
{
    static const char* e = "";
    return e;
}

#endif
