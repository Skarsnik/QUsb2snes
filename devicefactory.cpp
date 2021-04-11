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

#include "devicefactory.h"

DeviceFactory::DeviceFactory(QObject *parent) : QObject(parent)
{

}

QList<ADevice *> DeviceFactory::getDevices()
{
    return m_devices;
}

ADevice *DeviceFactory::attach(QString deviceName)
{
    m_attachError = "";
    foreach(ADevice* dev, m_devices)
    {
        if (dev->name() == deviceName)
            return dev;
    }
    //m_attachError = "Device named " + deviceName + " Not found";
    return nullptr;
}

QString DeviceFactory::attachError() const
{
    return m_attachError;
}

bool DeviceFactory::hasAsyncListDevices()
{
    return false;
}

QStringList DeviceFactory::getDevicesName() const
{
    return m_listDeviceNames;
}
