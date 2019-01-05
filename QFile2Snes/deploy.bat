@ECHO ON
set projectPath=F:\Project\QUsb2Snes\QFile2Snes
set compilePath=F:\Project\compile\qfile2snes
set deployPath=F:\Project\deploy\QFile2Snes
set originalBinDir=%compilePath%
set vscdll=F:\Visual Studio\VC\Redist\MSVC\14.12.25810\x64\Microsoft.VC141.CRT\msvcp140.dll

rmdir /Q /S %compilePath%
mkdir %compilePath%
rmdir /Q /S %deployPath%
mkdir %deployPath%
:: Compile

::"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

mkdir %compilePath%
cd %compilePath%
cd
::set QMAKE_MSC_VER=1910
qmake %projectPath%\QFile2Snes.pro  -spec win32-msvc "CONFIG+=release"
nmake

xcopy /y %originalBinDir%\release\QFile2Snes.exe %deployPath%

echo "Deploy QT"
windeployqt.exe --no-translations --no-system-d3d-compiler --no-opengl --no-svg --no-webkit --no-webkit2 --release %deployPath%\QFile2Snes.exe
xcopy /e %projectPath%\Patches %deployPath%\Patches\ /r

:: Clean up Qt extra stuff
rmdir /Q /S %deployPath%\imageformats
del %deployPath%\opengl32sw.dll
del %deployPath%\libEGL.dll
del %deployPath%\libGLESV2.dll
del %deployPath%\vc_redist.x64.exe

xcopy /y "%vscdll%" %deployPath%

::echo Generating Translation

::mkdir %deployPath%\i18n

::lrelease %projectPath%\Savestate2snes.pro
::xcopy /y %projectPath%\Translations\savestate2snes_fr.qm %deployPath%\i18n
::xcopy /y %projectPath%\Translations\savestate2snes_de.qm %deployPath%\i18n
::xcopy /y %projectPath%\Translations\savestate2snes_sv.qm %deployPath%\i18n
::xcopy /y %projectPath%\Translations\savestate2snes_nl.qm %deployPath%\i18n


xcopy /y %projectPath%\License-GPL3.txt %deployPath%
xcopy /i /y %projectPath%\README.md %deployPath%\Readme.txt
xcopy /y %projectPath%\icon50x50.png %deployPath%\icone.png


cd %projectPath%
