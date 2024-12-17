/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef USB2SNES_H
#define USB2SNES_H

#include <QObject>
#include <QStringList>

namespace SD2Snes {
Q_NAMESPACE

enum opcode {
    GET = 0,
    PUT,
    VGET,
    VPUT,

    LS,
    MKDIR,
    RM,
    MV,

    RESET,
    BOOT,
    POWER_CYCLE,
    INFO,
    MENU_RESET,
    STREAM,
    TIME,
    RESPONSE
};

Q_ENUM_NS(opcode)

enum  space {
    FILE = 0,
    SNES,
    MSU,
    CMD,
    CONFIG
};

Q_ENUM_NS(space)

enum  server_flags {
    NONE = 0,
    SKIPRESET = 1,
    ONLYRESET = 2,
    CLRX = 4,
    SETX = 8,
    STREAM_BURST = 16,
    NORESP = 64,
    DATA64B = 128,
};

Q_ENUM_NS(server_flags)

enum info_flags {
    FEAT_DSPX = 1,
    FEAT_ST0010 = 2,
    FEAT_SRTC = 4,
    FEAT_MSU1 = 8,
    FEAT_213F = 0x10,
    FEAT_CMD_UNLOCK = 0x20,
    FEAT_USB1 = 0x40,
    FEAT_DMA1 = 0x80
};

Q_ENUM_NS(info_flags)

enum class file_type {
    FILE = 1,
    DIRECTORY = 0
};

Q_ENUM_NS(file_type)

}

/*
 * Commands are send in JSON format
*/

namespace USB2SnesWS {
Q_NAMESPACE
    enum opcode {
    // Format is [argument to send]->what is returned, {} mean a result json reply
    // Size are in hexformat

    // Connection
    DeviceList, // List the available Device {portdevice1, portdevice2, portdevice3...}
    Attach, // Attach to the devise using the name [portdevice]
    AppVersion, // Give the version of the App {version}
    Name, // Specificy the name of the client [name]
    Close, // TODO this close the connection server side


    // Special
    Info, // Give information about the sd2snes firmware {firmwareversion, versionstring, romrunning, flag1, flag2...}TOFIX
    Boot, // Boot a rom [romname] TOTEST
    Menu, // Get back to the menu TOTEST
    Reset, // Reset TOTEST
    Binary, // TODO Send data directly to the sd2snes I guess?
    Stream, // TODO
    Fence, // TODO

    GetAddress, // Get the value of the address, space is important [offset, size]->datarequested TOFIX multiarg form
    PutAddress, // put value to the address  [offset, size] then send the binary data.
                // Also support multiple request in one [offset1, size1, offset2, size2] TOFIX work on size check/boundary
    PutIPS, // Apply a patch - [name, size] then send binary data
            // a special name is 'hook' for the sd2snes

    GetFile, // Get a file - [filepath]->{size}->filedata
    PutFile, // Post a file -  [filepath, size] then send the binary data
    List, // LS command - [dirpath]->{typefile1, namefile1, typefile2, namefile2...}
    Remove, // remove a file [filepath]
    Rename, // rename a file [filepath, newfilename]
    MakeDir, // create a directory [dirpath]

    QUsb2SnesRegisterApplication // Register an application to appear in QUsb2Snes device menu
                                 // [Name, short description, exec path, icon path]
    };
    Q_ENUM_NS(opcode)

    enum extra_info_flags {
        NO_FILE_CMD,
        NO_CONTROL_CMD,
        NO_ROM_WRITE,
        NO_ROM_READ
    };
    const unsigned int legacyPort = 8080;
    const unsigned int defaultPort = 23074;
    Q_ENUM_NS(extra_info_flags)
}

struct USB2SnesInfo {
    QString version;
    QString romPlaying;
    QString deviceName;
    QStringList flags;
};

#endif // USB2SNES_H
