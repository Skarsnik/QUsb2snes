# QUsb2snes

QUsb2Snes is a websocket server that provide an unified protocol for accessing hardware/software that act like a SNES (or are a SNES). A classic usage is to use the FileViewer client to upload roms to your SD2SNES.
But it allows for more advanced usage like reading/writing the memory of the SNES.

# Developpers

Look at the `usb2snes.h` file to have a detailed view of the command of websocket protocol.

# Users

## Usage

Download the latest release of QUsb2Snes (on the release link). Uncompress it somewhere and start QUsb2Snes.exe

## SD2Snes

You need to install the last Usb2Snes firmware by redguy (https://github.com/RedGuyyyy/sd2snes/releases/tag/v7) . Just follows the instruction provided. Don't start usb2snes.exe since QUsb2snes do the same thing that the original software. Your sd2snes device should show up on the Devices menu when connected.

## SNES9x multitroid

Multitroid provide a patched version of snes9x with a lua script that allow for external software to communicate with it. Get the emulator on http://multitroid.com/. Activate the snes9x bridge on the device menu of QUsb2Snes then run the multibridge.lua scripts on snes9x.

## RetroArch with Snes9x 2010 core

You need to use snes9x 2010 core and edit your `retroarch.cfg` file (can be found in %appData%\RetroArch) to set `network_cmd_enable = "true"` to true (default is false). Then you need to activate the RetroArch virtual device on the devices menu.

For software needing to the patch the ROM (multitroid for example) you etheir need to patch the rom manually with the IPS file or put the IPS file along side the rom with the same name for retroarch to auto patch it.

## RetroArch with other core

I need to patch RetroArch to be able to use any snes core x).

## SNES classic (called also SNES mini)

Only tested with Super Metroid.

Enable the SNES classic support on the device menu.

You need to 'hack' your SNES classic with the Hakchi2 CE version (https://github.com/TeamShinkansen/hakchi2/releases/) then remove the covershell mod (as explained in https://github.com/TeamShinkansen/hakchi2/releases/tag/v3.4.0). Install the `serverstuff` mod provided by QUsb2snes: copy the `serverstuff.mod` to the `user_mods` directory on Hakchi2, then install the mod with Hakchi2
Start the game and check if the SNES classic appear on the Devices menu, it should display `SNES classic : no client connected`. If not, try restarting the game.

It will not work with the 'normal' hakchi2 version as the Hakchi2 CE provide a more stable way to access the SNES Classic.

# Licence

QUsb2snes project follow the GPL version 3 licence, you can find the full version of the licence on the LICENCE file.
