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

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_sd2snesfact, "SD2SNESFactory")
#define sDebug() qCDebug(log_sd2snesfact())


#include <QSerialPortInfo>
#include <QTimer>

#include "sd2snesfactory.h"



SD2SnesFactory::SD2SnesFactory()
{

}


QStringList SD2SnesFactory::listDevices()
{
    QStringList toret;
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos)
    {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber();
        if (usbinfo.serialNumber() == "DEMO00000000")
        {
            bool isBusy = false;
            QString name = "SD2SNES " + usbinfo.portName();
            if (mapPortDev.contains(name) == false)
            {
                QSerialPort sPort(usbinfo);
                isBusy = !sPort.open(QIODevice::ReadWrite);
                sPort.close();
            }
            sDebug()  << "Used by other software " << isBusy;
            if (isBusy == false)
                toret << "SD2SNES " + usbinfo.portName();
        }
    }
    return toret;
}

ADevice *SD2SnesFactory::attach(QString deviceName)
{
    if (deviceName.left(3) != "COM" && deviceName.left(7) != "SD2SNES")
        return nullptr;

    // This is for apps that don't ask the device list
    if (deviceName.left(3) == "COM")
        deviceName = "SD2SNES " + deviceName;

    if (mapPortDev.contains(deviceName))
        return mapPortDev[deviceName];
    QString portName = deviceName.mid(8);
    SD2SnesDevice*  newDev = new SD2SnesDevice(portName);
    mapPortDev[deviceName] = newDev;
    m_devices.append(newDev);
    return newDev;
}

QString SD2SnesFactory::name() const
{
    return "SD2Snes";
}

bool SD2SnesFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Delete " << dev->name();
    mapPortDev.remove(dev->name());
    m_devices.removeAll(dev);
    dev->deleteLater();
    return true;
}

bool SD2SnesFactory::hasAsyncListDevices()
{
    return true;
}

bool SD2SnesFactory::devicesStatus()
{
    QTimer::singleShot(0, this , [=]{
        QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
        DeviceFactoryStatus status;
        status.status = Error::DeviceFactoryStatusEnum::DFS_SD2SNES_NO_DEVICE;
        status.generalError = Error::DeviceFactoryError::DFE_SD2SNES_NO_DEVICE;
        status.name = "SD2Snes";
        status.deviceNames.clear();
        status.deviceStatus.clear();
        foreach (QSerialPortInfo usbinfo, sinfos)
        {
            sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber();
            if (usbinfo.serialNumber() == "DEMO00000000")
            {
                bool isBusy = false;
                QString name = "SD2SNES " + usbinfo.portName();
                if (mapPortDev.contains(name) == false)
                {
                    QSerialPort sPort(usbinfo);
                    isBusy = !sPort.open(QIODevice::ReadWrite);
                    sPort.close();
                }
                sDebug() << "Used by other software " << isBusy;
                status.deviceNames.append(name);
                status.deviceStatus[name].state = ADevice::CLOSED;
                status.deviceStatus[name].error = isBusy ? Error::DeviceError::DE_SD2SNES_BUSY : Error::DeviceError::DE_NO_ERROR;
                status.status = Error::DeviceFactoryStatusEnum::DFS_SD2SNES_READY;
                status.generalError = Error::DeviceFactoryError::DFE_NO_ERROR;
            }
        }
        emit deviceStatusDone(status);
    });
    return true;
}


bool SD2SnesFactory::asyncListDevices()
{
    QTimer::singleShot(0, this , [=]{
        QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
        foreach (QSerialPortInfo usbinfo, sinfos)
        {
            sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber();
            if (usbinfo.serialNumber() == "DEMO00000000")
            {
                bool isBusy = false;
                QString name = "SD2SNES " + usbinfo.portName();
                if (mapPortDev.contains(name) == false)
                {
                    QSerialPort sPort(usbinfo);
                    isBusy = !sPort.open(QIODevice::ReadWrite);
                    sPort.close();
                    if (!isBusy)
                        emit newDeviceName("SD2SNES " + usbinfo.portName());
                    continue;
                }
                emit newDeviceName("SD2SNES " + usbinfo.portName());
            }
        }
        emit devicesListDone();
    });
    return true;
}
