nuget restore -PackagesDirectory packages
msbuild winuiapp.sln /P:Configuration=Debug
msbuild winuiapp.sln /P:Configuration=Release

rem winmd

copy .\packages\Microsoft.WindowsAppSDK.1.2.220930.4-preview2\lib\uap10.0\*.winmd ..\winmd
copy .\packages\Microsoft.WindowsAppSDK.1.2.220930.4-preview2\lib\uap10.0.18362\*.winmd ..\winmd
copy .\packages\Microsoft.Windows.SDK.Win32Metadata.34.0.8-preview\*.winmd ..\winmd
copy .\packages\Microsoft.Windows.SDK.Win32Metadata.34.0.8-preview\Windows.Win32.winmd ..\winmeta\winmetamoc
copy .\packages\Microsoft.Windows.SDK.Win32Metadata.34.0.8-preview\Windows.Win32.Interop.dll ..\winmeta\winmetamoc

rem headers

copy .\packages\Microsoft.WindowsAppSDK.1.2.220930.4-preview2\include\Microsoft.UI.Interop.h ..\idl

rem dlls

copy .\x64\Debug\winuiapp\CoreMessagingXP.dll ..\..\out\build\x64-Debug
copy .\x64\Debug\winuiapp\Microsoft.Internal.FrameworkUdk.dll ..\..\out\build\x64-Debug
copy .\x64\Debug\winuiapp\Microsoft.UI.Windowing.Core.dll ..\..\out\build\x64-Debug
copy .\x64\Debug\winuiapp\Microsoft.WindowsAppRuntime.dll ..\..\out\build\x64-Debug

copy .\x64\Debug\winuiapp\CoreMessagingXP.dll ..\..\out\build\x64-Release
copy .\x64\Debug\winuiapp\Microsoft.Internal.FrameworkUdk.dll ..\..\out\build\x64-Release
copy .\x64\Debug\winuiapp\Microsoft.UI.Windowing.Core.dll ..\..\out\build\x64-Release
copy .\x64\Debug\winuiapp\Microsoft.WindowsAppRuntime.dll ..\..\out\build\x64-Release

rem libs

copy .\packages\Microsoft.WindowsAppSDK.1.2.220930.4-preview2\lib\win10-arm64\Microsoft.WindowsAppRuntime.lib ..


