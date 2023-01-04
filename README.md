# QUsb2snes

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C/C++ CI](https://github.com/Skarsnik/QUsb2snes/actions/workflows/buildlinux.yml/badge.svg)](https://github.com/Skarsnik/QUsb2snes/actions/workflows/buildlinux.yml)
[![C/C++ CI](https://github.com/Skarsnik/QUsb2snes/actions/workflows/windows.yml/badge.svg)](https://github.com/Skarsnik/QUsb2snes/actions/workflows/windows.yml)
QUsb2Snes is a websocket server that provide an unified protocol for accessing hardware/software that act like a SNES (or are a SNES).
A classic usage is to use the FileViewer client to upload roms to your SD2SNES.
But it allows for more advanced usage like reading/writing the memory of the SNES.

## Installation

If you want the full experience go [Usb2Snes project](https://github.com/usb2snes/usb2snes/releases).
Otherwise, download the latest release of [QUsb2Snes](https://github.com/Skarsnik/QUsb2snes/releases).

This repository only have QUsb2Snes and a file viewer application.
You can also use the provided binaries from GitHub releases or [compile QUsb2Snes yourself from source](COMPILING.adoc).
On Linux, some distributions already ship qusb2snes (like arch AUR).

Installation via other means (homebrew, chocolatey, etc.) is not planned at the moment but may be provided in the future, if there is demand.

---

## Usage

### Configuration files

Logs are put in the standard path for application data.
It should be: `$HOME/.local/share/QUsb2Snes` on Linux and `%APPDATA/QUsb2Snes` on Windows.

Please note that the directory will be named after the binary file. If you create a symlink on Linux (e.g. qusb2snes.exe), the directory will change accordingly.

And the settings file is into
`$HOME/.config/skarsnik.nyo.fr/QUsb2Snes.conf`

### Enabling/Disabling emulators

The GUI options are reflected in the configuration file.

For example, to enable the retroarchdevice, add or change the config under `[General]`:

```ini
[General]
retroarchdevice=true
```

### Serial configuration (Linux)

To make your sd2snes device work, you will need to set some tty setting

`stty -F /dev/ttyACM0 0:0:cbd:0:3:1c:7f:15:4:5:40:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0`

Replace `/dev/ttyACM0` with the correct device.


### Emulator configuration

#### SD2Snes

You need to install the last [Usb2Snes firmware by Redguyyy](https://github.com/RedGuyyyy/sd2snes/releases/).
Just follows the instruction provided.
Don't start `usb2snes.exe` since QUsb2snes do the same thing that the original software.
Your sd2snes device should show up on the Devices menu when connected.


#### SNES9x Rerecording

Snes9x-rr allows to run Lua scripts in the emulator, that allows the emulator to connect to QUsb2snes. You can use the Multitroid version of Snes9x-rr (http://multitroid.com/) or use Snes9x-rr 1.60 https://github.com/gocha/snes9x-rr/releases. Older version of Snes9x-rr does not allow to write to ROM from Lua so some application will not work.

Activate the `Lua bridge` on the device menu of QUsb2Snes.
Then run the luabridge.lua script from the LuaBridge folder from the Lua console window in Snes9x


#### BizHawk 2.3.1 (bsnes core recommanded)

In the device menu of QUsb2Snes activate the `Lua bridge`

You will need to change the Lua support on the emulator. Go into the `Config -> Customize` menu then goes into the Advanced tab and select `Lua+LuaInterface` at the bottom in the Lua core part. Restart the emulator.
In the Lua directory of BizHawk create a `lua_bridge` directory (or a similar name) then copy the content of the LuaBridge directory from QUsb2Snes (the lua file and the dll file).
Run your game and then in the `Tools` menu start the `Lua console` click on the folder icon to load the `multibridge.lua` file. You need to close the Lua console if you want to disconnect properly.


#### RetroArch with Snes9x core

You can use Snex9x (not recommanded) or bsnes-mercury cores. You need to activate the network command support, etheir in the configuration menu of RetroArch or editing your `retroarch.cfg` file (can be found in %appData%\RetroArch) to set `network_cmd_enable = "true"` (default is false). Then you need to activate the RetroArch virtual device on the devices menu. Any flavor of bsnes-mercury is prefered as we can access the ROM data.

Snex9x core : For software needing to the patch the ROM (multitroid for example) you etheir need to patch the rom manually with the IPS file or put the IPS file along side the rom with the same name for retroarch to auto patch it.

You can connect to a remote RetroArch by adding a `RetroArchHosts="remoteName=ip"` in the config file. If you want to add multiple hosts, just add ; between each host definition.


#### SNES classic (called also SNES mini)

If for some reason your SNES classic does not have the expected IP address (connected via wifi or something) you can add a `SNESClassicIP=myip` in the config.ini file.


#### Native emulator (canoe)

Mostly tested with Super Metroid.

Enable the SNES classic support on the device menu.

You need to 'hack' your SNES classic with the Hakchi2 CE version (https://github.com/TeamShinkansen/hakchi2/releases/) then remove the covershell mod if needed (as explained in https://github.com/TeamShinkansen/hakchi2/releases/tag/v3.4.0).

Install the `serverstuff` mod provided by QUsb2snes: copy the `serverstuff.hmod` to the `user_mods` directory on Hakchi2, then install the mod with Hakchi2

Start the game and check if the SNES classic appear on the Devices menu, it should display something like `SNES classic : no client connected`. If not, try restarting the game.

It will not work with the 'normal' Hakchi2 version as the Hakchi2 CE provide a more stable way to access the SNES Classic.


#### RetroArch

Enable the RetroArch support via the context menu, by changing the corresponding option in the config file to `true`, or by starting a NOGUI compiled version with the `retroarch` argument.
Open Hakchi2 CE and in the menu go to 'tools -> open ftp client' it should open your default browser, copy paste the url it should look like `ftp://169.254.13.37/` for the default ip, and paste in the file explorer to go to it (if it ask you for user/password, just put root as user, there is no password).
Go into the `/etc/libretro/` folder and copy the `retroarch.cfg` file somewhere.
Open the copy with a text editor and search for the `network_cmd_enable` entry, replace `false` with `true`, save.
Then now put your copy in place of the original one.


#### BSnes-AS

Activate the `Lua bridge` on the device menu of QUsb2Snes.
In the `Script` menu of Bsnes-as, select `Load Script` and pick the usb2snes.as file from the included scripts folder.

### Command line options

#### Emulator choice

This is only available in conjunction with the `-nogui` option.

Only the non gui version take arguments, It still use the config file but you can use some arguments to enable the support you want.

* `-sd2snes` : to activate the sd2snes support
* `-luabridge` : for the lua bridge support
* `-retroarch` : for the retroarch support
* `-snesclassic` :  for the snes classic support

---

## Compiling from source

See [COMPILING.adoc](COMPILING.adoc).

## Developers

Look at the [docs/Protocol.md](docs/Protocol.md) file to have a detailed view of the command of websocket protocol.

## Licence

QUsb2snes project follow the GPL version 3 licence, you can find the full version of the licence on the LICENCE file.

## Icons

* Flat style icons are from google https://material.io/tools/icons/?style=baseline and under Apache Licence 2.0
* Snes9x and RetroArch icon are from their respective project
* QUsb2Snes icon is from TrenteR_TR on FrankerFaceZ with changed contrast/light.
* Most pony icons are from FrankerFaceZ.
