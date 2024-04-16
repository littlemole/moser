cmake --preset win-x64-debug  || goto :error
cmake --build --preset win-x64-debug || goto :error

cmake --preset win-x64-release  || goto :error
cmake --build --preset win-x64-release || goto :error


cd win32

msbuild  xmoser.vcxproj -t:build -p:Configuration=Debug || goto :error
msbuild  xmoser.vcxproj -t:build -p:Configuration=Release || goto :error

cd ..

bin\precompile.bat || goto :error

exit 

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%

