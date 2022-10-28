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

#ifndef LUABRIDGE_H
#define LUABRIDGE_H

#include "../devicefactory.h"
#include "luabridgedevice.h"
#include <QRandomGenerator>

#include <QObject>
#include <QTcpServer>

class LuaBridge : public DeviceFactory
{
public:
    LuaBridge();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool deleteDevice(ADevice *);
    QString name() const;
    bool asyncListDevices();
    bool devicesStatus();
    bool hasAsyncListDevices();


private slots:
    void    onNewConnection();
    void    onClientDisconnected();

private:
    QStringList                 allocatedNames;
    QTcpServer*                 server;
    QList<QTcpSocket*>          clients;
    QRandomGenerator*           rng;
    quint32                     rngSeed;
    QMap<QTcpSocket*, LuaBridgeDevice*> mapSockDev;

};

#endif // LUABRIDGE_H
