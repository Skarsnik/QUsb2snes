/*
 * Copyright (c) 2021 the QUsb2Snes authors.
 *
 * This file is part of QUsb2Snes.
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

#ifndef SD2SNESFACTORY_H
#define SD2SNESFACTORY_H

#include <QMap>
#include <QObject>

#include "../devicefactory.h"
#include "sd2snesdevice.h"

class SD2SnesFactory : public DeviceFactory
{
    Q_OBJECT
public:
    SD2SnesFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    QString status();
    QString name() const;
    bool deleteDevice(ADevice *);
    bool    hasAsyncListDevices();

private:
    QMap<QString, SD2SnesDevice*>   mapPortDev;

    // DeviceFactory interface
public:
    bool asyncListDevices();
};

#endif // SD2SNESFACTORY_H
