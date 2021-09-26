/*
 * Copyright (c) 2021 Sylvain "Skarsnik" Colinet.
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
    case DE_RETROARCH_NO_CONNECTION:
        return QObject::tr("Can't connect to host");
    default: return QString();
    }
}

QString deviceErrorHints(DeviceError err)
{
    switch (err)
    {
    case DE_RETROARCH_NO_CONNECTION:
        return QObject::tr("RetroArch not running or network command support not activated");
    default:
        return QString();
    }
}

}
