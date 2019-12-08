For now QUsb2Snes Linux integration is not great. It need work on other application detection.

# Compiling

You need the developpment files for Qt 5.8 or supperior.
You will need the developplent files for websocket and serialport libraries that goes with your Qt version
You can etheir use the Qbs build tool or the regular qmake build too to build it. QMake is simpler to use.

```
qmake QUsb2snes.pro CONFIG+='release'
make
```

Then start the QUsb2Snes binary from the `release` directory that is created.

# Usage

There is a -nogui option if you want to run it without the system tray icon and interface.

# Files

Logs are put in the standard path for application data. It should be:
~/.loca/share/QUsb2Snes

And the settings file is into
~/.config/skarsnik.nyo.fr/QUsb2Snes.conf



# SD2Snes

To make your sd2snes device work, you will need to set some tty setting 

`stty -F /dev/ttyACM0 0:0:cbd:0:3:1c:7f:15:4:5:40:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0`

Replace `/dev/ttyACM0` by the correct device.


