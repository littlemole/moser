cd win32

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
rem done
rem *************

exit /b %errorlevel%

:error

rem *************
rem goto hell
rem *************

echo Failed with error #%errorlevel%.
exit /b %errorlevel%
