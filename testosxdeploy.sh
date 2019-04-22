make clean
export PATH="/Users/piko/Qt/5.12.2/clang_64/bin/:$PATH"
qmake
make
cd QFile2Snes
qmake
make
cd ..
mkdir QUsb2Snes.app/Contents/MacOs/apps
mkdir QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/
cp QFile2Snes/QFile2Snes.app/Contents/MacOs/QFile2Snes QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/
cp QFile2Snes/icon50x50.png QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/icone.png
cp QFile2Snes/qusb2snesapp.json QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/
macdeployqt QUsb2Snes.app -dmg

