cd win32

msbuild  xmoser.vcproj -t:build -p:Configuration=Debug || goto :error
msbuild  xmoser.vcproj -t:build -p:Configuration=Release || goto :error

cd ..

bin\precompile.bat || goto :error

exit 

:error
echo Failed with error #%errorlevel%.
exit /b %errorlevel%

