@echo off

rem *************
rem init vcpkg
rem *************

if not exist vcpkg (
git clone https://github.com/microsoft/vcpkg.git || goto :error
cd vcpkg
cmd /C bootstrap-vcpkg.bat || goto :error
cd ..
)

rem *************
rem prepare dirs
rem *************


if not exist "out" mkdir out
cd out
if not exist "build" mkdir build
cd build
if not exist "x64-Debug" mkdir "x64-Debug"
if not exist "x64-Release" mkdir "x64-Release"
cd ..
cd ..

cd lib
cd win
if not exist "rt" mkdir rt
if not exist "win32" mkdir win32
cd ..
cd ..

cd win32

if not exist "winmd" mkdir winmd
if not exist "idl" mkdir idl

cd winmeta
if not exist "lib" mkdir lib
cd lib
if not exist "rt" mkdir rt
cd ..

cd winmetamoc
if not exist "lib" mkdir lib
cd lib
if not exist "win32" mkdir win32
cd ..
cd ..

cd ..
cd ..

rem *************
rem cmake build
rem *************

cmake --preset win-x64-debug  || goto :error
cmake --build --preset win-x64-debug || goto :error

cmake --preset win-x64-release  || goto :error
cmake --build --preset win-x64-release || goto :error


rem *************
rem restore nuget
rem *************

cd win32

msbuild xmoser.vcxproj -t:restore -p:RestorePackagesConfig=true || goto :error

rem *************
rem build moxaml
rem *************


cd moxaml

msbuild  moxaml.vcxproj -t:build -p:Configuration=Debug || goto :error
msbuild  moxaml.vcxproj -t:build -p:Configuration=Release || goto :error

cd ..

rem *************
rem build xmoser
rem *************


msbuild  xmoser.vcxproj -t:build -p:Configuration=Debug || goto :error
msbuild  xmoser.vcxproj -t:build -p:Configuration=Release || goto :error

rem *************
rem copy vcredist
rem *************


copy /Y "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x64\Microsoft.VC143.CRT\*.dll ../out/ || goto :error
copy /Y "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x64\Microsoft.VC143.CXXAMP\*.dll ../out/ || goto :error
copy /Y "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.38.33135\x64\Microsoft.VC143.OpenMP\*.dll ../out/ || goto :error

rem *************
rem copy winmds
rem *************

copy ..\out\build\x64-Release\moxaml.winmd winmd || goto :error
copy .\packages\Microsoft.WindowsAppSDK.1.5.240311000\lib\uap10.0\*.winmd winmd || goto :error
copy .\packages\Microsoft.WindowsAppSDK.1.5.240311000\lib\uap10.0.17763\*.winmd winmd || goto :error
copy .\packages\Microsoft.Windows.SDK.Win32Metadata.59.0.13-preview\*.winmd winmd || goto :error


copy .\packages\Microsoft.Windows.SDK.Win32Metadata.59.0.13-preview\Windows.Win32.winmd winmeta\winmetamoc || goto :error
rem copy .\packages\Microsoft.Windows.SDK.Win32Metadata.59.0.13-preview\Windows.Win32.Interop.dll winmeta\winmetamoc || goto :error


rem *************
rem build res.dll
rem *************

cd mocres

msbuild mocres.sln /P:Configuration=Debug || goto :error
msbuild mocres.sln /P:Configuration=Release || goto :error

cd ..

rem *************
rem build meta parsers
rem *************


cd winmeta
copy ..\winmd\*.winmd . || goto :error

msbuild winmeta.sln /P:Configuration=Release || goto :error

if not exist "lib" mkdir lib
if not exist "lib\rt" mkdir lib\rt

rem *************
rem gen winrt lib
rem *************

cmd /C gen.bat || goto :error

copy lib\rt\*.lib ..\..\lib\win\rt || goto :error

rem *************
rem gen win32 lib
rem *************


cd winmetamoc
if not exist "lib" mkdir lib
if not exist "lib\win32" mkdir lib\win32
cmd /C gen.bat || goto :error

rem *************
rem update lib
rem *************

copy lib\win32\*.lib ..\..\..\lib\win\win32 || goto :error


cd ..

cd ..

cd ..

rem *************
rem precompile libs
rem *************

bin\precompile.bat

rem *************
rem done
rem *************

exit /b %errorlevel%

:error

rem *************
rem goto hell
rem *************

echo Failed with error #%errorlevel%.
exit /b %errorlevel%
