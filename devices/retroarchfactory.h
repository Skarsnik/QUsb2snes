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
#include "retroarchdevice.h"


class RetroArchFactory : public DeviceFactory
{
    Q_OBJECT
    struct RAHost {
        RAHost() {
            sock = nullptr;
            port = 55355;
            device = nullptr;
        }
        QString             name;
        QHostAddress        addr;
        QUdpSocket*         sock;
        quint16             port;
        QString             status;
        RetroArchDevice*    device;
    };

public:
    RetroArchFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool deleteDevice(ADevice *dev);
    QString status();
    QString name() const;
    ADevice* attach(QString deviceName);


private slots:
    void    onUdpDisconnected();

private:
    QMap<QString, RAHost>           raHosts;

    bool            checkRetroArchHost(RAHost& host);

    bool            tryNewRetroArchHost(RAHost& host);

    // DeviceFactory interface
public:
    bool asyncListDevices();
};

#endif // RETROARCHFACTORY_H
