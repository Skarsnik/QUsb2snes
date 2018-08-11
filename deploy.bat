
set deployPath=D:\Project\build-QUsb2snes-Desktop_Qt_5_11_0_MSVC2017_64bit-Release\qtc_Desktop_Qt_5_11_0_MSVC2017_64bit_Release\install-root
set projectPath=D:\Project\QUsb2snes\
set usb2snes=D:\Romhacking\usb2snes_v7\usb2snes_v7
set magic2snes=D:\Project\deploy\Magic2Snes

cd %deployPath%

echo "Deploy QT"
windeployqt.exe --no-translations --no-system-d3d-compiler --no-opengl-sw --no-opengl --no-webkit --no-patchqt --no-webkit2 --release  %deployPath%\QUsb2Snes.exe

:: Clean up Qt extra stuff
::rmdir /Q /S %deployPath%\imageformats
::del %deployPath%\Qt5core.dll
::xcopy /y C:\Qt\Qt5.11.0\5.11.0\msvc2017_64\bin\Qt5core.dll %deployPath%
del %deployPath%\libEGL.dll
del %deployPath%\libGLESV2.dll
del %deployPath%\vc_redist.x64.exe
::xcopy %deployPath% "D:\VM shared\qubs2snes" /syq
rmdir /Q /S %deployPath%\apps
mkdir %deployPath%\apps
mkdir %deployPath%\Magic2Snes
xcopy %usb2snes%\apps %deployPath%\apps /syq
xcopy %magic2snes% %deployPath%\Magic2Snes /syq

cd %projectPath%