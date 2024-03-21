@echo off
if not exist vcpkg (
git clone https://github.com/microsoft/vcpkg.git || goto :error
cd vcpkg
cmd /C bootstrap-vcpkg.bat || goto :error
cd ..
)


if not exist "out" mkdir out
cd out
if not exist "build" mkdir build
cd build
if not exist "x64-Debug" mkdir "x64-Debug"
if not exist "x64-Release" mkdir "x64-Release"
cd ..
cd ..

cd win32

if not exist "winmd" mkdir winmd
if not exist "idl" mkdir idl

cd winuidep
cmd /C prepare.bat || goto :error
cd ..

cd winmd
cmd /c gen.bat || goto :error
cd ..

cd idl
cmd /c gen.bat || goto :error
cd ..

cd mocres
msbuild mocres.sln /P:Configuration=Debug || goto :error
msbuild mocres.sln /P:Configuration=Release || goto :error
cd ..

cd moxaml
del /s /q packages
nuget restore || goto :error
rem msbuild moxaml.sln /P:Configuration=Debug /T:Clean
rem msbuild moxaml.sln /P:Configuration=Release /T:Clean
msbuild moxaml.sln /P:Configuration=Debug || goto :error
msbuild moxaml.sln /P:Configuration=Release || goto :error
copy ..\..\out\build\x64-Release\moxaml.winmd ..\winmd || goto :error
copy .\packages\Microsoft.Web.WebView2.1.0.1264.42\lib\*.winmd ..\winmd || goto :error
copy .\packages\Microsoft.UI.Xaml.2.8.0-prerelease.220712001\lib\uap10.0\*.winmd ..\winmd || goto :error

cd ..

cd winmeta
copy ..\winmd\*.winmd .
msbuild winmeta.sln /P:Configuration=Release
if not exist "lib" mkdir lib
if not exist "lib\rt" mkdir lib\rt
cmd /C gen.bat
copy lib\rt\*.lib ..\..\lib\win\rt

cd winmetamoc
if not exist "lib" mkdir lib
if not exist "lib\win32" mkdir lib\win32
cmd /C gen.bat
copy lib\win32\*.lib ..\..\..\lib\win\win32

cd ..

cd ..

cd ..


rem set CMAKE_TOOLCHAIN_FILE=%~dp0vcpkg\scripts\toolchains\windows.cmake
set CMAKE_TOOLCHAIN_FILE=%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake
echo CMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE%

cmd /C bin\build.bat || goto :error

exit /b %errorlevel%

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
