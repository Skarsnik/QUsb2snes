# QUsb2snes

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C/C++ CI](https://github.com/Skarsnik/QUsb2snes/actions/workflows/buildlinux.yml/badge.svg)](https://github.com/Skarsnik/QUsb2snes/actions/workflows/buildlinux.yml)
[![Build status](https://ci.appveyor.com/api/projects/status/r8t2hpt21ux5r7mi/branch/master?svg=true)](https://ci.appveyor.com/project/Skarsnik/qusb2snes/branch/master)

QUsb2Snes is a websocket server that provide an unified protocol for accessing hardware/software that act like a SNES (or are a SNES).
A classic usage is to use the FileViewer client to upload roms to your SD2SNES.
But it allows for more advanced usage like reading/writing the memory of the SNES.

QUsb2Snes is also a choice to add multiworld support to the _Legend of Zelda 3: A Link to the Past Randomizer_ game, or to attach the _OpenTracker_ with automatic tracking. QUsb2Snes will act as a bridge between your SNES or Emulator of choice and other programs.

## Installation

If you want the full experience go [Usb2Snes project](https://github.com/usb2snes/usb2snes/releases).
Otherwise, download the latest release of [QUsb2Snes](https://github.com/Skarsnik/QUsb2snes/releases).

This repository only have QUsb2Snes and a file viewer application.
You can also use the provided binaries from GitHub releases or [compile QUsb2Snes yourself from source](COMPILING.adoc).
On Linux, some distributions already ship qusb2snes (like arch AUR).

Installation via other means (homebrew, chocolatey, etc.) is not planned at the moment but may be provided in the future, if there is demand.

## Usage

See [USAGE.adoc](USAGE.adoc).

## Compiling from source

See [COMPILING.adoc](COMPILING.adoc).

## Developers

Look at the `docs/Protocol.md` file to have a detailed view of the command of websocket protocol.

## Licence

QUsb2snes project follow the GPL version 3 licence, you can find the full version of the licence on the LICENCE file.

## Icons

* Flat style icons are from google https://material.io/tools/icons/?style=baseline and under Apache Licence 2.0
* Snes9x and RetroArch icon are from their respective project
* QUsb2Snes icon is from TrenteR_TR on FrankerFaceZ with changed contrast/light.
* Most pony icons are from FrankerFaceZ.
