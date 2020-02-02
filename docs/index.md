---
layout: default
title: QUsb2Snes
---

Welcome to QUsb2Snes documentation.

QUsb2Snes is a reimplementation of the Usb2Snes websocket server using a more multi plateform framework (Qt, ench the Q in the name) than the original one.

You should look at http://usb2snes.com/ if you are regular user.

# Contents
{:.no_toc}

* TOC
{:toc}

# Origin

## Usb2Snes firmware

To make more thing more clear of what is exactly QUsb2Snes, let's get back to the origin of the Usb2Snes project.

Usb2Snes is a firmware for the Sd2snes cartridge written by Redguyy that allows to use the USB port on the Sd2Snes to access various fonctionnality of the Sd2Snes. Most notable are uploading ROM, read and write various memories of the console.

The issue with this approche, when writing application that use this techology only one application can have access to the Sd2snes. So for example you cannot run an Item tracker for alttp randomizer at the same time than the fileviewer to upload a new ROM.

![Direct access](images/directaccess.png)

To avoid this issue, Redguyy wrote an intermediary application also called Usb2Snes (enjoy your name confusion) that other application use to talk to the Sd2Snes.

So a basic usage look like that:

![Websocket access](images/wsaccess.png)

## Emulators, SNES Classic...

QUsb2Snes is a replacement for the middle part of the previous diagram. Since now the application
does not access directly the Sd2Snes through the Usb connection. Nothing prevent this intermediate part to trick the applications to make them access something else, like a Snes Emulator or a SNES Classic

For example Snes9x

![Lua connection](images/luaconnection.png)


# Support

QUsb2Snes currently support:

* SD2Snes with the [usb2snes](https://github.com/RedGuyyyy/sd2snes/releases) firmware.
* [Snes9x multitroid](https://drive.google.com/open?id=1_ej-pwWtCAHYXIrvs5Hro16A1s9Hi3Jz) with Lua support
* SNES classic moded with [Hakchi2 CE](https://github.com/TeamShinkansen/hakchi2/releases)
* RetroArch support with Snes9x and bsnes-mercury cores

# Application

A list of applications and what they do is available on the [application page](https://skarsnik.github.io/QUsb2snes/Application)

# Magic2Snes

Magic2Snes is a special application that make writing applications for Usb2Snes easier for non developpers. You will find more informations on [Magic2Snes](https://github.com/Skarsnik/Magic2snes/wiki)

By default QUsb2Snes comes with various Magic2Snes scripts.

# Installation

## Windows

* Download the lastest release on the [release page](https://github.com/Skarsnik/QUsb2snes/releases)
* Uncompress the 7zip file and start the QUsb2Snes executable

## Mac os X

* Download the lastest release (dmg file) on the [release page](https://github.com/Skarsnik/QUsb2snes/releases)

SD2Snes devices may have issue on mac ox X

## Linux

For now you will have to compile the source yourself, see the `LinuxREADME.md` file to how to do it.

### Arch Linux
There now exists a PKGBUILD for the upstream version of QUsb2Snes which also provides QFile2Snes (https://aur.archlinux.org/packages/qusb2snes-git/)

# Usage

If you don't use a SD2Snes, you will have to activate the support for your way to play SNES in the `devices` menu.

## SD2Snes

If you came from usb2snes.com, the firmware should be provided directly on the bundle.

You need to install the last Usb2Snes firmware by Redguy (https://github.com/RedGuyyyy/sd2snes/releases/tag/usb2snes_v11) . Just follows the instruction provided. Don't start usb2snes.exe since QUsb2snes do the same thing that the original software. Your sd2snes device should show up on the Devices menu when connected.

## SNES9x Rerecording

Snes9x-rr allows to run Lua scripts in the emulator, that allows use to connect to QUsb2snes. You can use the Multitroid version of Snes9x-rr (http://multitroid.com/) or use Snes9x-rr 1.60 https://github.com/gocha/snes9x-rr/releases. Older version of Snes9x-rr does not allow to write to ROM from Lua so some application will not work.

Activate the `Lua bridge` on the device menu of QUsb2Snes.
Then run the luabridge.lua script from the LuaBridge folder from the Lua console window in Snes9x

## BizHawk 2.3.1 (bsnes core recommanded)

In the device menu of QUsb2Snes activate the `lua bridge`

You will need to change the Lua support on the emulator. Go into the `Config -> Customize` menu then goes into the Advanced tab and select `Lua+LuaInterface` at the bottom in the Lua core part. Restart the emulator.
In the Lua directory of BizHawk create a `lua_bridge` directory (or a similar name) then copy the content of the BizHawk directory from QUsb2Snes (the lua file and the dll file).
Run your game and then in the `Tools` menu start the `Lua console` click on the folder icon to load the `multibridge.lua` file. You need to close the Lua console if you want to disconnect properly.


## RetroArch

You can use Snex9x (not recommanded) or bsnes-mercury cores. You need to activate the network command support, etheir in the configuration menu of RetroArch or editing your retroarch.cfg file (can be found in %appData%\RetroArch) to set network_cmd_enable = "true" (default is false). Then you need to activate the RetroArch virtual device on the devices menu. Any flavor of bsnes-mercury is prefered as we can access the ROM data.

Snex9x core : For software needing to the patch the ROM (multitroid for example) you etheir need to patch the rom manually with the IPS file or put the IPS file along side the rom with the same name for retroarch to auto patch it.

You can connect to a remote RetroArch by adding a RetroArchHost=YourOwnHost in the config file, replace YourOwnHost with the right ip/hostname.

## SNES classic (called also SNES mini)

Only tested with Super Metroid. This also only work with the native emulator (called canoe), if you use RetroArch on your snes classic, please refer to the RetroArch section.

Enable the SNES classic support on the device menu.

You need to 'hack' your SNES classic with the Hakchi2 CE version (https://github.com/TeamShinkansen/hakchi2/releases/) then remove the covershell mod if needed (as explained in https://github.com/TeamShinkansen/hakchi2/releases/tag/v3.4.0). 

Install the `serverstuff` mod provided by QUsb2snes: copy the `serverstuff.hmod` to the `user_mods` directory on Hakchi2, then install the mod with Hakchi2

Start the game and check if the SNES classic appear on the Devices menu, it should display something like `SNES classic : no client connected`. If not, try restarting the game.

It will not work with the 'normal' Hakchi2 version as the Hakchi2 CE provide a more stable way to access the SNES Classic.

If for some reason your SNES classic does not have the expected IP address (connected via wifi or something) you can add a `SNESClassicIP=myip` in the config.ini file.
