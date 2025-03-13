#ifdef ENABLE_MOSER_XAML
#pragma once

#include "targetver.h"
//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// This tells App.xaml.g.hpp the app will define its own WinMain
#define DISABLE_XAML_GENERATED_MAIN

// There's an API named GetCurrentTime in the Storyboard type.
#undef GetCurrentTime

// Com and WinRT headers
#include <Unknwn.h>

#endif

