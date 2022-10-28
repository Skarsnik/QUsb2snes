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
    QString name() const;
    bool devicesStatus();

    enum class StatusState {
        NO_CHECK,
        CHECK_ALIVE,
        CHECK_PIDCANOE,
        CHECK_CANOE_VERSION,
        CHECK_CANOE_MODE,
        CHECK_MEMORY_LOCATION_READ_RAM_LOC,
        CHECK_MEMORY_LOCATION_PMAP,
        CHECK_MEMORY_LOCATION_READ_ROM_CHECK1,
        CHECK_MEMORY_LOCATION_READ_ROM_CHECK2
    };
    Q_ENUM(StatusState)

private:
    QTcpSocket*         socket = nullptr;
    QByteArray          canoePid;
    QByteArray          oldCanoePid;
    QByteArray          dataRecv;
    SNESClassic*        device = nullptr;
    QTimer              checkAliveTimer;
    unsigned int        romLocation = 0;
    unsigned int        sramLocation = 0;
    unsigned int        ramLocation = 0;
    unsigned int        lastPmapLocation = 0;
    unsigned int        readMemSize;
    QString             snesclassicIP;
    bool                checkingState;
    bool                doingDeviceList;
    bool                doingDeviceStatus;
    bool                doingCommand;
    StatusState         checkState;
    DeviceFactoryStatus factStatus;

    void    executeCommand(QByteArray toExec);
    void    writeSocket(QByteArray toWrite);
    /*QByteArray readCommandReturns(QTcpSocket *msocket);
    QByteArray readSocketReturns(QTcpSocket* msocket);*/
    bool    checkStuff();
    void    aliveCheck();

    bool    hasValidMemory();

    void    resetMemoryAddresses();

    void    onReadyRead();
    void    onSocketConnected();
    void    onSocketError(QAbstractSocket::SocketError err);

    void    checkSuccess();
    void    checkFailed(Error::DeviceFactoryError err, QString extra = QString());

public:
    bool    asyncListDevices();
    bool    hasAsyncListDevices();

};

#endif // SNESCLASSICFACTORY_H
