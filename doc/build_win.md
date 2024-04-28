## building MOSER on windows

note: x64 only!

# prerequisites

- Visual Studio latest
- C++ & C# Desktop + UWP workloads
- Windows SDK v 10.0.19041.0 (enable in VS installer!)
- Git (Visual Studio version or standalone)
- Cmake (Visual Studio version or standalone)

# building

open a x64 visual studo console window. make sure it is x64!

fist build:

- git clone https://github.com/littlemole/moser.git
- cd moser
- msbuild build.xml -t:bootstrap

this will build MOSER and all of the freakin Win32 specific support stuff (win32 and winRT API projections, XAML support, WindowsAppSDK support, etc - see win32 subfolder)

subsequent builds:

- msbuild build.xml -t:build

look at build.xml for all the available msbuild targets.

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
 