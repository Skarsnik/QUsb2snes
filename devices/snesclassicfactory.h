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

#ifndef SNESCLASSICFACTORY_H
#define SNESCLASSICFACTORY_H

#include <QObject>
#include <QTcpSocket>

#include "../devicefactory.h"

#include "snesclassic.h"

class SNESClassicFactory : public DeviceFactory
{
    Q_OBJECT
public:
    SNESClassicFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString status();
    QString name() const;
private:
    QTcpSocket*         socket = nullptr;
    QByteArray          canoePid;
    SNESClassic*        device = nullptr;
    QTimer              checkAliveTimer;
    unsigned int        romLocation = 0;
    unsigned int        sramLocation = 0;
    unsigned int        ramLocation = 0;
    QString             snesclassicIP;

    void executeCommand(QByteArray toExec);
    void writeSocket(QByteArray toWrite);
    QByteArray readCommandReturns(QTcpSocket *msocket);
    QByteArray readSocketReturns(QTcpSocket* msocket);

    void findMemoryLocations();
    bool checkStuff();
    void aliveCheck();

    bool hasValidMemory();
    void resetMemoryAddresses();

    // DeviceFactory interface
public:
    bool asyncListDevices();
};

#endif // SNESCLASSICFACTORY_H
