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

#ifndef RETROARCHFACTORY_H
#define RETROARCHFACTORY_H

#include <QObject>
#include <QUdpSocket>
#include "../devicefactory.h"
#include "retroarchhost.h"
#include "retroarchdevice.h"


class RetroArchFactory : public DeviceFactory
{
    Q_OBJECT
    struct HostData {
        HostData() {
            device = nullptr;
        }
        QString             name;
        RetroArchHost*      host;
        bool                error;
        RetroArchDevice*    device;
        qint64              reqId;
    };

public:
    RetroArchFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool    hasAsyncListDevices();
    bool asyncListDevices();
    bool deleteDevice(ADevice *dev);
    QString name() const;
    ADevice* attach(QString deviceName);
    bool devicesStatus();


private slots:
    void    onRaHostInfosDone(qint64 id);
    void    onRaHostgetInfosFailed(qint64 id);
    void    onRaHostErrorOccured(QAbstractSocket::SocketError err);

private:
    QMap<QString, HostData> raHosts;
    int                     hostCheckCount;
    int                     hostChecked;

    void                    checkInfoDone();
    bool                    doingListDevices;
    bool                    doingDevicesStatus;
    DeviceFactoryStatus     m_status;

/*    bool            checkRetroArchHost(RAHost& host);

    bool            tryNewRetroArchHost(RAHost& host);

    RAHost          testRetroArchHost(RAHost host);*/

    void    addHost(RetroArchHost *host);
    void    checkDevices();
};

#endif // RETROARCHFACTORY_H
