For now QUsb2Snes Linux integration is pretty bad (especially configuration file)

# Compiling

You need the developpment files for Qt 5.8 or supperior.
You will need the developplent files for websocket and serialport libraries that goes with your Qt version
You can etheir use the Qbs build tool or the regular qmake build too to build it. QMake is simpler to use.

```
qmake QUsb2Snes.pro CONFIG+='release'
make
```

Then start the QUsb2Snes binart from the `release` directory that is created.

# SD2Snes

To make your sd2snes device work, you will need to set some tty setting 

`stty -F /dev/ttyACM0 0:0:cbd:0:3:1c:7f:15:4:5:40:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0`

Replace `/dev/ttyACM0` by the correct device.


