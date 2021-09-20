
#include "deviceerror.h"

namespace Error {

QString deviceFactoryErrorString(DeviceFactoryError er)
{
    switch(er)
    {
    case DFE_SD2SNES_NO_DEVICE : return QObject::tr("No SD2Snes/FXpak pro device detected");
    case DFE_LUA_CANT_LISTEN  : return QObject::tr("Lua bridge support can't open a connection");
    default: return QString();
    }
}

QString deviceFactoryStatusString(DeviceFactoryStatusEnum st)
{
    switch (st)
    {
    case DFS_SD2SNES_READY : return QObject::tr("SD2Snes/Fxpak pro device ready");
    case DFS_LUA_LISTENNING: return QObject::tr("Waiting for emulator to connect");
    default: return QString();
    }
}

QString deviceErrorString(DeviceError er)
{
    switch(er)
    {
    case DE_RETROARCH_UNREACHABLE:
        return QObject::tr("Host unreachable");
    default: return QString();
    }
}

}
