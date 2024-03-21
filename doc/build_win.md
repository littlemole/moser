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

- git clone TODO: moser
- cd moser
- bin/bootstrap.bat

subsequent builds:

- bin/build.bat

this will build MOSER and all of the freakin Win32 specific support stuff (win32 and winRT API projections, XAML support, WindowsAppSDK support, etc - see win32 subfolder)

to (re-)build just the core MOSER, user cmake presets provided (should work with both visual studio and visual studio code).
 