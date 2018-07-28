
set deployPath=D:\Project\build-QUsb2snes-Desktop_Qt_5_11_0_MSVC2017_64bit-Release\qtc_Desktop_Qt_5_11_0_MSVC2017_64bit_Release\install-root\
set projectPath=D:\Project\QUsb2snes\

cd %deployPath%

echo "Deploy QT"
windeployqt.exe --no-translations --no-system-d3d-compiler --no-opengl --no-svg --no-webkit --no-webkit2 --compiler-runtime --release  %deployPath%\QUsb2Snes.exe

:: Clean up Qt extra stuff
rmdir /Q /S %deployPath%\imageformats
::This shit is  24 Meg
del %deployPath%\opengl32sw.dll 
del %deployPath%\Qt5core.dll
xcopy /y C:\Qt\Qt5.11.0\5.11.0\msvc2017_64\bin\Qt5core.dll %deployPath%
::del %deployPath%\libEGL.dll
::del %deployPath%\libGLESV2.dll


cd %projectPath%