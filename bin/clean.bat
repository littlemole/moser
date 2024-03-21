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
del /s /q /f win32\winmeta\metamoc\x64
rmdir /s /q win32\winmeta\metamoc\x64
del /s /q /f win32\winmeta\winmetamoc\x64
rmdir /s /q win32\winmeta\winmetamoc\x64
del /s /q /f "win32\winuidep\Generated Files"
rmdir /s /q "win32\winuidep\Generated Files"
del /s /q /f win32\winuidep\packages
rmdir /s /q win32\winuidep\packages
del /s /q /f win32\winuidep\x64
rmdir /s /q win32\winuidep\x64
del /s /q /f win32\winuidep\winuiapp
rmdir /s /q win32\winuidep\winuiapp

del /S *.mbc
