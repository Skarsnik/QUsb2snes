#
# This file is part of the QUsb2Snes project.
#
# QUsb2Snes is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# QUsb2Snes is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
#

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

