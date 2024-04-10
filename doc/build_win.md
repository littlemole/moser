## building MOSER on windows

note: x64 only!

# prerequisites

- Visual Studio latest
- C++ Desktop + UWP workloads
- Python and Git (Visual Studio versions ok)
- Windows SDK v 10.0.19041.0 
- Cmake
- nuget.exe in path

# building

open a x64 visual studo console window. make sure it is x64.

fist build:

- git clone https://github.com/littlemole/moser.git
- cd moser
- bin/bootstrap.bat

subsequent builds:

- bin/build.bat

this will build MOSER and all of the freakin Win32 specific support stuff (win32 and winRT API projections, XAML support, WindowsAppSDK support, etc - see win32 subfolder)

# PATIENCE !

the windows build will take some time, especially on first bootstrap.
the build will

- initialize local vcpkg (takes some time)
- build vcpkg dependencies (also takes some time)
- do a cmake build of moser.exe (quick)
- restore win32 nuget packages (quick)
- build the xaml support moxaml.dll (long winrtcpp build)
- build the xmoser.exe with WinUI3 support (even longer winrtcpp build)
- copy vcredist deps (quick)
- copy winmd metadata (quick)
- build the example resource only mocres.dll (quick)
- build the meta data parsers for winrt winmd and win32 winmd (takes a bit)
- generate winrt projection from winmd files (takes forever - go for lunch)
- generate win32 projection (coffee time!)
- precompile the generates libs (quite fast)



to (re-)build just the core MOSER, user cmake presets provided (should work with both visual studio and visual studio code).
 