cd lib
del /S *.mbc
for /R %%f in (*.lib) do ..\out\build\x64-Release\xmoser.exe -c "%%f"
cd ..
xcopy  lib\* out\build\x64-Debug\lib /E /Y /I
xcopy  lib\* out\build\x64-Release\lib /E /Y /I
