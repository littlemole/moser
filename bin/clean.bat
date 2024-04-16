rem @echo off

del /s /q /f vcpkg
rmdir /s /q vcpkg
del /s /q /f out
rmdir /s /q out
del /s /q /f win32\mocres\mocres\x64
rmdir /s /q win32\mocres\mocres\x64
del /s /q /f win32\moxaml\obj
rmdir /s /q win32\moxaml\obj
del /s /q /f win32\moxaml\packages
rmdir /s /q win32\moxaml\packages
del /s /q /f win32\moxaml\AppPackages
rmdir /s /q win32\moxaml\AppPackages
del /s /q /f win32\winmeta\x64
rmdir /s /q win32\winmeta\x64
del /s /q /f win32\winmeta\lib
del /s /q /f win32\winmeta\*.winmd
del /s /q /f win32\winmeta\metamoc\x64
del /s /q /f win32\winmeta\winmetamoc\*.winmd
del /s /q /f win32\winmeta\winmetamoc\*.dll
del /s /q /f win32\winmeta\winmetamoc\lib
rmdir /s /q win32\winmeta\metamoc\x64
del /s /q /f win32\winmeta\winmetamoc\x64
rmdir /s /q win32\winmeta\winmetamoc\x64

del /s /q /f win32\obj
rmdir /s /q win32\obj

del /S *.mbc

del /s /q /f win32\wix\bin
del /s /q /f win32\wix\moser
del /s /q /f win32\wix\obj
del /s /q /f win32\wix\moserSetup.msi
del /s /q /f win32\wix\moserPerUserSetup.msi

