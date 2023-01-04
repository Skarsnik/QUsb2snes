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
#include <QSettings>
#include <QtEndian>

Q_LOGGING_CATEGORY(log_snesclassicfact, "SNESClassic Factory")
#define sDebug() qCDebug(log_snesclassicfact)
#define sInfo() qCInfo(log_snesclassicfact)

extern QSettings* globalSettings;
extern bool    dontLogNext;

#define SNES_CLASSIC_IP "169.254.13.37"

#include "snesclassic.h"
#include "snesclassicfactory.h"

SNESClassicFactory::SNESClassicFactory()
    : snesclassicIP(SNES_CLASSIC_IP)
{
    socket = new QTcpSocket(this);

    if (globalSettings->contains("SNESClassicIP"))
    {
        snesclassicIP = globalSettings->value("SNESClassicIP").toString();
    }

    sInfo() << "SNES Classic device will try to connect to " << snesclassicIP;
    checkState = StatusState::NO_CHECK;
    checkAliveTimer.setInterval(5000);
    doingDeviceList = false;
    doingDeviceStatus = false;
    connect(&checkAliveTimer, &QTimer::timeout, this, &SNESClassicFactory::aliveCheck);
    connect(socket, &QAbstractSocket::readyRead, this, &SNESClassicFactory::onReadyRead);
    connect(socket, &QAbstractSocket::connected, this, &SNESClassicFactory::onSocketConnected);
    connect(socket, &QAbstractSocket::stateChanged, this, [=] {
        sDebug() << "Socket state changed" << socket->state();
    });
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    connect(socket, &QAbstractSocket::errorOccurred, this, &SNESClassicFactory::onSocketError);
#else
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &SNESClassicFactory::onSocketError);
#endif
}

void    SNESClassicFactory::onSocketConnected()
{
    sDebug() << "Connected";
    if ((doingDeviceList || doingDeviceStatus) && checkState == StatusState::NO_CHECK)
    {
        checkStuff();
    }
}

void SNESClassicFactory::onSocketError(QAbstractSocket::SocketError err)
{
    sDebug() << "Socket error" << err << socket->errorString();
    if (err == QAbstractSocket::ConnectionRefusedError || err == QAbstractSocket::NetworkError)
    {
        if (doingDeviceList || doingDeviceStatus)
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_NO_DEVICE);
    }
}


void SNESClassicFactory::executeCommand(QByteArray toExec)
{
    doingCommand = true;
    sDebug() << "Executing : " << toExec;
    writeSocket("CMD " + toExec + "\n");
}

void SNESClassicFactory::writeSocket(QByteArray toWrite)
{
    sDebug() << ">>" << toWrite;
    socket->write(toWrite);
}


void SNESClassicFactory::onReadyRead()
{
    quint64 size = socket->bytesAvailable();
    dataRecv += socket->readAll();
    sDebug() << "Received : " << size << " bytes" << dataRecv;
    if (doingCommand)
    {
        if (dataRecv.right(10) == "\0\0ERROR\0\0")
        {
            sDebug() << "Error with the command, abording";
            //TODO stop the check;
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_COMMAND_ERROR);
            doingCommand = false;
            return;
        }
        if (dataRecv.right(4) != QByteArray(4, 0))
        {
            return;
        } else {
            sDebug() << "Command finished, triming the 4 trailing 0";
            doingCommand = false;
            dataRecv.chop(4);
        }
    } else {
        if (dataRecv.size() != readMemSize)
        {
            return ;
        } else {
            readMemSize = 0;
        }
    }
    switch(checkState)
    {
    case StatusState::CHECK_ALIVE:
    {
        dontLogNext = false;
        dataRecv = dataRecv.trimmed();
        // Canoe not running anymore
        if (dataRecv.isEmpty())
        {
            sDebug() << "Closing the device, Canoe not running anymore";
            device->close();
            socket->close();

            checkAliveTimer.stop();
            resetMemoryAddresses();
            checkState = StatusState::NO_CHECK;
            break;
        }
        canoePid = dataRecv;
        // Canoe still running, everything should be fine
        //sDebug() << canoePid << oldCanoePid;
        if (canoePid == oldCanoePid)
        {
            //sDebug() << "Pid is same, nice";
            checkState = StatusState::NO_CHECK;
            break;
        }
        // We have a new pid, the old addresses are invalid
        resetMemoryAddresses();
        checkStuff();
        break;
    }
    case StatusState::CHECK_PIDCANOE:
    {
        if (dataRecv.isEmpty())
        {
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_CANOE_NOT_RUNNING);
            break;
        }
        canoePid = dataRecv.trimmed();
        oldCanoePid = canoePid;
        executeCommand("canoe-shvc --version");
        checkState = StatusState::CHECK_CANOE_VERSION;
        break;
    }
    case StatusState::CHECK_CANOE_VERSION:
    {
        if (QString(dataRecv) != "fc349ac43140141277d2d6f964c1ee361fcd20ca\n")
        {
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_WRONG_VERSION, dataRecv);
            m_attachError = tr("SNES Classic emulator is not v2.0.14 - has hash " + dataRecv);
            break;
        }
        if (oldCanoePid != canoePid)
            resetMemoryAddresses();
        // canoe in demo mode is useless
        executeCommand("ps | grep canoe-shvc | grep -v grep");
        checkState = StatusState::CHECK_CANOE_MODE;
        break;
    }
    case StatusState::CHECK_CANOE_MODE:
    {
        if (dataRecv.isEmpty())
        {
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_CANOE_NOT_RUNNING);
            break;
        }
        if (dataRecv.indexOf("-resume") != -1)
        {
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_DEMO_MODE);
            m_attachError = tr("SNES Classic emulator is running in demo mode.");
            break;
        }
        if (!hasValidMemory())
        {
            checkState = StatusState::CHECK_MEMORY_LOCATION_READ_RAM_LOC;
            bool     ok;
            uint32_t pid = canoePid.toULong(&ok);
            //(*0x1dff84) + 0x20BEC
            QString s = QString::asprintf("READ_MEM %u %zx %u\n", pid, 0x1dff84, 4);
            readMemSize = 4;
            writeSocket(s.toUtf8());
        } else {
            checkSuccess();
        }
        break;
    }
    case StatusState::CHECK_MEMORY_LOCATION_READ_RAM_LOC:
    {
        ramLocation = qFromLittleEndian<quint32>(static_cast<const void*>(dataRecv.constData())) + 0x20BEC;
        executeCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx");
        checkState = StatusState::CHECK_MEMORY_LOCATION_PMAP;
        break;
    }
    case StatusState::CHECK_MEMORY_LOCATION_PMAP:
    {
        QList<QByteArray> memEntries = dataRecv.split('\n');

        romLocation = 0;
        sramLocation = 0;
        bool ok;
        for (const QByteArray& memEntry : qAsConst(memEntries))
        {
            sDebug() << memEntry;
            if (memEntry.isEmpty())
                continue;
            QString s = memEntry;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14 ,0)
            QStringList ls = s.split(" ", Qt::SkipEmptyParts);
#else
            QStringList ls = s.split(" ", QString::SkipEmptyParts);
#endif

            if (ls.at(1) == "5092")
            {
                sramLocation = ls.at(0).toULong(&ok, 16) + 0x26E0;
            }
            else if (ls.at(1) == "8196")
            {
                romLocation = ls.at(0).toULong(&ok, 16) + 0x38;
            }
            else if (ls.at(1) == "10240")
            {
                // The 8196KB rom block is sometimes combined with a 2044KB stack block,
                // so check for the rom 2044KB into this block.
                // The proper rom location will start with 5 4-byte values:
                // 0x00: 0
                // 0x04: 8392706
                // 0x08: 256
                // 0x0C: <rom size>
                // 0x10: 48
                bool ok;
                uint32_t pid = canoePid.toULong(&ok);
                unsigned int location = ls.at(0).toULong(&ok, 16);
                lastPmapLocation = location;
                QString s = QString::asprintf("READ_MEM %u %x %u\n", pid, location + 2044 * 1024, 20);
                readMemSize = 20;
                writeSocket(s.toUtf8());
                checkState = StatusState::CHECK_MEMORY_LOCATION_READ_ROM_CHECK1;
            }
        }
        sDebug() << "Mem location" << sramLocation << romLocation << ramLocation;
        if (hasValidMemory())
            checkSuccess();
        break;
    }
    case StatusState::CHECK_MEMORY_LOCATION_READ_ROM_CHECK1:
    {
        const char* data = dataRecv.constData();
        if (qFromLittleEndian<uint32_t>(data + 4) == 8392706
            && qFromLittleEndian<uint32_t>(data + 8) == 256
            && qFromLittleEndian<uint32_t>(data + 16) == 48)
        {
            romLocation = lastPmapLocation + 2044 * 1024 + 0x38;
            checkSuccess();
        } else {
        // If it wasn't there, also check at the start of this block
            bool ok;
            uint32_t pid = canoePid.toULong(&ok);
            QString s = QString::asprintf("READ_MEM %u %x %u\n", pid, lastPmapLocation, 20);
            readMemSize = 20;
            writeSocket(s.toUtf8());
            checkState = StatusState::CHECK_MEMORY_LOCATION_READ_ROM_CHECK2;
        }
        break;
    }
    case StatusState::CHECK_MEMORY_LOCATION_READ_ROM_CHECK2:
    {
        const char* data = dataRecv.constData();
        if (qFromLittleEndian<uint32_t>(data + 4) == 8392706
            && qFromLittleEndian<uint32_t>(data + 8) == 256
            && qFromLittleEndian<uint32_t>(data + 16) == 48)
        {
            romLocation = lastPmapLocation + 0x38;
            checkSuccess();
        } else {
            checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_MEMORY_LOCATION_NOT_FOUND);
        }
    }
    }
    dataRecv.clear();
}

void SNESClassicFactory::checkSuccess()
{
    checkState = StatusState::NO_CHECK;
    sDebug() << "Check success DL - DS" << doingDeviceList << doingDeviceStatus;
    if (doingDeviceList)
    {
        doingDeviceList = false;
        if (device == nullptr)
        {
            sDebug() << "Creating SNES Classic device";
            device = new SNESClassic();
            device->canoePid = canoePid;
            device->setMemoryLocation(ramLocation, sramLocation, romLocation);
            m_devices.append(device);
        }
        if (device->state() == ADevice::CLOSED)
        {
            device->sockConnect(snesclassicIP);
            checkAliveTimer.start();
        }
        emit newDeviceName(device->name());
        emit devicesListDone();
    }
    if (doingDeviceStatus)
    {
        doingDeviceStatus = false;
        factStatus.status = Error::DeviceFactoryStatusEnum::DFS_SNESCLASSIC_READY;
        emit deviceStatusDone(factStatus);
    }
    // This is probably not needed for every case, but this ensure
    // the checkalive canoe can refresh the device settings
    if (device != nullptr)
    {
        device->canoePid = canoePid;
        device->setMemoryLocation(ramLocation, sramLocation, romLocation);
    }
}

void    SNESClassicFactory::checkFailed(Error::DeviceFactoryError err, QString extra)
{
    checkState = StatusState::NO_CHECK;
    sDebug() << "Check failed " << err;
    if (doingDeviceList)
    {
        doingDeviceList = false;
        emit devicesListDone();
    }
    if (doingDeviceStatus)
    {
        factStatus.generalError = err;
        if (!extra.isEmpty())
            ;
        doingDeviceStatus = false;
        emit deviceStatusDone(factStatus);
    }
}


bool SNESClassicFactory::checkStuff()
{
    sDebug() << "Checkstuff called" << checkState;
    if (checkState == StatusState::NO_CHECK)
    {
        checkState = StatusState::CHECK_PIDCANOE;
        executeCommand("pidof canoe-shvc");
        QTimer::singleShot(200, this, [=] {
            if (checkState == StatusState::CHECK_PIDCANOE)
            {
                checkFailed(Error::DFE_SNESCLASSIC_NO_DEVICE, "Connected but canoe check timeout");
            }
        });
    }
    if (checkState == StatusState::CHECK_ALIVE)
    {
        executeCommand("pidof canoe-shvc");
    }
    return true;
}

void SNESClassicFactory::aliveCheck()
{
    if (checkState == StatusState::NO_CHECK)
    {
        dontLogNext = true;
        checkState = StatusState::CHECK_ALIVE;
        executeCommand("pidof canoe-shvc");
     }
}

QStringList SNESClassicFactory::listDevices()
{
    QStringList toret;
    return toret;
}


ADevice *SNESClassicFactory::attach(QString deviceName)
{
    m_attachError = "";
    if (deviceName != "SNES Classic")
    {
        return nullptr;
    }
    if (device != nullptr)
    {
        sDebug() << "Attach" << device->state();
    }
    if (device != nullptr && device->state() != ADevice::CLOSED)
    {
        return device;
    }
    return nullptr;
}

bool SNESClassicFactory::deleteDevice(ADevice *)
{
    return false;
}

QString SNESClassicFactory::name() const
{
    return "SNES Classic (Hakchi2CE)";
}

bool SNESClassicFactory::asyncListDevices()
{
    sDebug() << "List devices";
    doingDeviceList = true;
    if (socket->state() == QAbstractSocket::ConnectingState)
    {
        sDebug() << "This should not happen, socket already trying to connect";
        return false;
    }
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        sDebug() << "Trying to connect to serverstuff";
        QTimer::singleShot(0, this, [=] {
            socket->connectToHost(snesclassicIP, 1042);}
        );
        QTimer::singleShot(200, this, [=] {
           sDebug() << "Timeout " << socket->state();
           if (socket->state() == QAbstractSocket::ConnectingState)
           {
               socket->close();
               this->checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_NO_DEVICE);
           }
        });
    } else {
        checkStuff();
    }
    return true;
}


bool SNESClassicFactory::devicesStatus()
{
    sDebug() << "devices Status";
    factStatus.name = "SNES Classic";
    factStatus.generalError = Error::DFE_NO_ERROR;
    factStatus.status = Error::DFS_SNESCLASSIC_NO_DEVICE;
    factStatus.deviceNames.clear();
    factStatus.deviceStatus.clear();

    if (device != nullptr && device->state() == ADevice::BUSY)
    {
        factStatus.deviceNames.append("SNES Classic");
        factStatus.status = Error::DFS_SNESCLASSIC_READY;
        QTimer::singleShot(0, this, [=] {
            emit deviceStatusDone(factStatus);}
        );
        return true;
    }
    doingDeviceStatus = true;
    if (socket->state() == QAbstractSocket::ConnectingState)
    {
        sDebug() << "This should not happen, socket already trying to connect";
        checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_NO_DEVICE);
        return false;
    }
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        sDebug() << "Trying to connect to serverstuff";
        QTimer::singleShot(0, this, [=] {
            socket->connectToHost(snesclassicIP, 1042);}
        );
        QTimer::singleShot(200, this, [=] {
           sDebug() << "Timeout " << socket->state();
           if (socket->state() == QAbstractSocket::ConnectingState)
           {
               socket->close();
               this->checkFailed(Error::DeviceFactoryError::DFE_SNESCLASSIC_NO_DEVICE);
           }
        });
    } else {
        checkStuff();
    }
    return true;
}

bool SNESClassicFactory::hasValidMemory()
{
    return ramLocation != 0 && romLocation != 0 && sramLocation != 0;
}

void SNESClassicFactory::resetMemoryAddresses()
{
    ramLocation = 0;
    romLocation = 0;
    sramLocation = 0;
    if (device)
    {
        device->setMemoryLocation(ramLocation, sramLocation, romLocation);
    }
}



bool SNESClassicFactory::hasAsyncListDevices()
{
    return true;
}
