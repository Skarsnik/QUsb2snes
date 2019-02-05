For now QUsb2Snes Linux integration is pretty bad (especially configuration file)

# Compiling

You need the developpment files for Qt 5.8 or supperior. You also need Qbs
You will need the developplent files for websocket and serialport libraries that goes with your Qt version

Then you need to setup Qbs

`qbs-setup-qt --dectect`

This should create a profile named like qt5-8.5 (corresponding to the Qt version). Use this profile to finally compile QUsb2Snes.

`qbs profile:qt-5-8.5 build`

Then start QUsb2Snes from the `default/install-root` directory that is created


