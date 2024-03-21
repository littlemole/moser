cd out\build\x64-Release
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=%~dp0\..\vcpkg\scripts\buildsystems\vcpkg.cmake ../../.. || goto :error
cmake --build . || goto :error
cd ..
cd ..
cd ..
bin\precompile.bat || goto :error

exit 

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%

