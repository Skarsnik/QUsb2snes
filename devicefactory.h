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

#ifndef DEVICEFACTORY_H
#define DEVICEFACTORY_H

#include <QObject>
#include "adevice.h"

class DeviceFactory : public QObject
{
    Q_OBJECT
public:
    /*
    struct DeviceInfo {
        ADevice *device;

    };*/
    enum DeviceFactoryState {
        NONE,
        BUSYDEVICELIST,
        BUSYATTACH
    };

    explicit DeviceFactory(QObject *parent = nullptr);
    virtual QStringList listDevices() = 0;
    QList<ADevice*>     getDevices();
    virtual ADevice*    attach(QString deviceName);
    virtual bool        deleteDevice(ADevice*) = 0;
    QString             attachError() const;
    virtual QString     status() = 0;
    virtual QString     name() const = 0;
    virtual bool        hasAsyncListDevices();
    virtual bool        asyncListDevices() = 0;
    virtual QStringList getDevicesName() const;
    //QList<DeviceInfo>   deviceInfos();

signals:
    void    deviceRemoved(ADevice*);
    void    newDeviceName(QString name);
    void    devicesListDone();


protected:
    QList<ADevice*> m_devices;
    QString         m_attachError;
    DeviceFactoryState  m_state;
    QStringList     m_listDeviceNames;
};

#endif // DEVICEFACTORY_H
