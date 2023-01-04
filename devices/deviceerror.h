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

#ifndef DEVICEERROR_H
#define DEVICEERROR_H

#include <QString>
#include <QObject>

namespace Error {
Q_NAMESPACE

enum DeviceFactoryError {
    DFE_NO_ERROR,
    DFE_SD2SNES_NO_DEVICE,
    DFE_LUA_CANT_LISTEN,
    DFE_SNESCLASSIC_NO_DEVICE,
    DFE_SNESCLASSIC_CANOE_NOT_RUNNING,
    DFE_SNESCLASSIC_COMMAND_ERROR,
    DFE_SNESCLASSIC_WRONG_VERSION,
    DFE_SNESCLASSIC_DEMO_MODE,
    DFE_SNESCLASSIC_MEMORY_LOCATION_NOT_FOUND,
    DFE_EMUNWA_NO_CLIENT,
};

Q_ENUM_NS(DeviceFactoryError)

enum DeviceFactoryStatusEnum {
    DFS_SD2SNES_NO_DEVICE,
    DFS_SD2SNES_READY,
    DFS_LUA_LISTENNING,
    DFS_RETROARCH_NO_RA,
    DFS_RETROARCH_NO_ALL,
    DFS_SNESCLASSIC_NO_DEVICE,
    DFS_SNESCLASSIC_NOT_USABLE,
    DFS_SNESCLASSIC_READY,
    DFS_EMUNWA_NO_CLIENT,
    DFS_EMUNWA_READY
};

Q_ENUM_NS(DeviceFactoryStatusEnum)

QString deviceFactoryErrorString(DeviceFactoryError er);
QString deviceFactoryErrorHints(DeviceFactoryError er);
QString deviceFactoryStatusString(DeviceFactoryStatusEnum st);

enum DeviceError {
    DE_NO_ERROR,
    DE_SD2SNES_BUSY,
    DE_RETROARCH_INFO_FAILED,
    DE_RETROARCH_UNREACHABLE,
    DE_RETROARCH_NO_CONNECTION,
    DE_RETROARCH_NO_VERSION,
    DE_RETROARCH_NO_SNES_GAME,
    DE_RETROARCH_NO_WRAM_READ,
    DE_RETROARCH_NO_ROM_INFO,
    DE_RETROARCH_NO_GAME,
    DE_RETROARCH_TIMEOUT,
    DE_EMUNWA_NO_CLIENT,
    DE_EMUNWA_INCOMPATIBLE_CLIENT,
    DE_EMUNWA_NO_SNES_CORE
};

Q_ENUM_NS(DeviceError)

QString deviceErrorString(DeviceError er);
QString deviceErrorHints(DeviceError er);

}
#endif // DEVICEERROR_H
