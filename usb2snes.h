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

    // Special
    Info, // Give information about the sd2snes firmware {firmwareversion, versionstring, romrunning, flag1, flag2...}TOFIX
    Boot, // Boot a rom [romname] TODO
    Menu, // Get back to the menu TODO
    Reset, // Reset TODO
    Stream, // TODO
    Fence, // TODO

    GetAddress, // Get the value of the address TODO
    PutAddress, // put value to the address TODO
    PutIPS, // Apply a patch TODO

    GetFile, // Get a file [filepath]->{size}->filedata
    PutFile, // Post a file [filepath, size]-> TODO
    List, // LS command [dirpath]->{typefile1, namefile1, typefile2, namefile2...}
    Remove, // remove a file
    Rename, // rename a file
    MakeDir // create a directory
};
Q_ENUM_NS(opcode)
}

struct USB2SnesInfo {
    QString version;
    QString romPlaying;
    QStringList flags;
};

#endif // USB2SNES_H
