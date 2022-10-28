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

#ifndef EMUNETWORKACCESSFACTORY_H
#define EMUNETWORKACCESSFACTORY_H

#include <QObject>
#include "../devicefactory.h"
#include "emunetworkaccessdevice.h"
#include "emunwaccessclient.h"

class EmuNetworkAccessFactory : public DeviceFactory
{
    Q_OBJECT

public:
    EmuNetworkAccessFactory();

    enum DetectState {
        NO_CHECK,
        CHECK_CONNECTION,
        DOING_NAME,
        CHECK_EMU_INFO,
        CHECK_CORE_LIST,
    };
    Q_ENUM(DetectState)

public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString name() const;
    bool hasAsyncListDevices();
    bool asyncListDevices();
    bool devicesStatus();

private:
    struct ClientInfo {
        EmuNWAccessClient*      client;
        EmuNetworkAccessDevice* device;
        QString                 deviceName;
        enum DetectState        checkState;
        bool                    doingAttach;
        int                     port;
    };
    unsigned int checkedCount;
    bool    doingDeviceStatus;
    bool    doingDeviceList;
    DeviceFactoryStatus  devFacStatus;
    unsigned short          startingPort;

    void    checkStatus();
    void    checkFailed(EmuNWAccessClient* client, Error::DeviceError);
    void    checkSuccess(EmuNWAccessClient* client);

    // DeviceFactory interface

    QMap<EmuNWAccessClient*, ClientInfo>    clientInfos;

 private slots:
    void    onClientDisconnected();
    void    onClientConnected();
    void    onClientConnectionError();
    void    onClientReadyRead();

};

#endif // EMUNETWORKACCESSFACTORY_H
