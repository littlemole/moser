#ifndef moc_dlopen_h
#define moc_dlopen_h

#ifdef _WIN32

/*
    helpers for Win32 so we can use std POSIX syntax to load shared libraries (DLLs)
*/

void* dlopen(const char* file, int mode);

int dlclose(void* handle);

void* dlsym(void* handle, const char* name);

const char* dlerror();
#else
#include <dlfcn.h>
#endif

#endif
