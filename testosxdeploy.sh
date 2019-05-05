make clean
rm -fr QUsb2Snes.app
export PATH="/Users/piko/Qt/5.12.2/clang_64/bin/:$PATH"
qmake
make
cd QFile2Snes
make clean
qmake
make
cd ..
mkdir QUsb2Snes.app/Contents/MacOs/apps
mkdir QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/
cp QFile2Snes/QFile2Snes.app/Contents/MacOs/QFile2Snes QUsb2Snes.app/Contents/MacOs/
cp QFile2Snes/icon50x50.png QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/icone.png
cp QFile2Snes/qusb2snesapp.json QUsb2Snes.app/Contents/MacOs/apps/QFile2Snes/
rm QUsb2Snes.dmg
macdeployqt QUsb2Snes.app -dmg -executable="QUsb2Snes.app/Contents/MacOs/QFile2Snes"

