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
    socket = new QTcpSocket();

    if (globalSettings->contains("SNESClassicIP"))
    {
        snesclassicIP = globalSettings->value("SNESClassicIP").toString();
    }

    sInfo() << "SNES Classic device will try to connect to " << snesclassicIP;

    checkAliveTimer.setInterval(1000);
    connect(&checkAliveTimer, &QTimer::timeout, this, &SNESClassicFactory::aliveCheck);
}

void SNESClassicFactory::executeCommand(QByteArray toExec)
{
    sDebug() << "Executing : " << toExec;
    writeSocket("CMD " + toExec + "\n");
}

void SNESClassicFactory::writeSocket(QByteArray toWrite)
{
    sDebug() << ">>" << toWrite;
    socket->write(toWrite);
}

QByteArray SNESClassicFactory::readCommandReturns(QTcpSocket* msocket)
{
    QByteArray toret = readSocketReturns(msocket);
    toret.truncate(toret.size() - 4);
    return toret;
}

QByteArray SNESClassicFactory::readSocketReturns(QTcpSocket* msocket)
{
    QByteArray toret;
    msocket->waitForReadyRead(50);
    forever {
        QByteArray data = msocket->readAll();
        sDebug() << "Reading" << data;
        if (data.isEmpty())
        {
            break;
        }
        if (toret.isEmpty() && data.left(4) == QByteArray(4, 0))
            data = data.mid(4);
        toret += data;
        if (!msocket->waitForReadyRead(100))
        {
            break;
        }
    }
    return toret;
}

void SNESClassicFactory::findMemoryLocations()
{
    QByteArray pmap;
    executeCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    pmap = readCommandReturns(socket);
    QList<QByteArray> memEntries = pmap.split('\n');

    resetMemoryAddresses();

    bool ok;
    uint32_t pid = canoePid.toULong(&ok);
    QString s;
    //(*0x1dff84) + 0x20BEC
    s.sprintf("READ_MEM %u %zx %u\n", pid, 0x1dff84, 4);
    writeSocket(s.toUtf8());
    QByteArray memory = readSocketReturns(socket);
    ramLocation = qFromLittleEndian<uint32_t>(memory.data()) + 0x20BEC;

    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);

        if (ls.at(1) == "5092")
        {
            sramLocation = ls.at(0).toULong(&ok, 16) + 0x26E0;
        }
        else if (ls.at(1) == "8196")
        {
            romLocation = ls.at(0).toULong(&ok, 16) + 0x38;
        }
    }
    sDebug() << "Locations : ram/sram/rom" << QString::number(ramLocation, 16) << QString::number(sramLocation, 16) << QString::number(romLocation, 16);
}

bool SNESClassicFactory::checkStuff()
{
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    if (!data.isEmpty())
    {
        executeCommand("canoe-shvc --version");
        QByteArray version = readCommandReturns(socket);
        if(QString(version) != "fc349ac43140141277d2d6f964c1ee361fcd20ca\n")
        {
            m_attachError = tr("SNES Classic emulator is not v2.0.14 - has hash " + version);
            return 0;
        }

        auto oldCanoePid = canoePid;
        canoePid = data.trimmed();
        if(oldCanoePid != canoePid)
        {
            resetMemoryAddresses();
        }
        // canoe in demo mode is useless
        executeCommand("ps | grep canoe-shvc | grep -v grep");
        QByteArray canoeArgs = readCommandReturns(socket);
        if (canoeArgs.indexOf("-resume") != -1)
        {
            m_attachError = tr("SNES Classic emulator is running in demo mode.");
            return false;
        }

        // Try to connect with canoe if we haven't already
        if (!hasValidMemory())
        {
            findMemoryLocations();
        }
        // If we still don't have valid memory, bail
        if (ramLocation == 0 || romLocation == 0 || sramLocation == 0)
        {
            m_attachError = tr("Can't find memory locations, try restarting the game.");
            return false;
        }
        else
        {
            return true;
        }
    } else {
        m_attachError = tr("The SNES Classic emulator is not running.");
    }
    return false;
}

void SNESClassicFactory::aliveCheck()
{
    QByteArray oldPid = canoePid;
    dontLogNext = true;
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    dontLogNext = false;
    data = data.trimmed();
    // Canoe not running anymore
    if (data.isEmpty())
    {
        sDebug() << "Closing the device, Canoe not running anymore";
        device->close();

        checkAliveTimer.stop();
        resetMemoryAddresses();
        return;
    }
    canoePid = data.trimmed();
    // Canoe still running, everything should be fine
    if (canoePid == oldPid)
    {
        return;
    }
    // We have a new pid, the old addresses are invalid
    resetMemoryAddresses();
    if (checkStuff())
    {
        sDebug() << "Updating device infos";
        device->canoePid = canoePid;

    } else {
        sDebug() << "Closing the device, can't find location";
        device->close();
    }
}

QStringList SNESClassicFactory::listDevices()
{
    QStringList toret;
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        sDebug() << "Trying to connect to serverstuff";
        socket->connectToHost(snesclassicIP, 1042);
        socket->waitForConnected(100);
    }
    sDebug() << socket->state();
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        if (checkStuff())
        {
            if (device == nullptr)
            {
                sDebug() << "Creating SNES Classic device";
                device = new SNESClassic();
                m_devices.append(device);
            }
            if (device->state() == ADevice::CLOSED)
            {
                device->sockConnect(snesclassicIP);
                checkAliveTimer.start();
            }
            device->canoePid = canoePid;
            device->setMemoryLocation(ramLocation, sramLocation, romLocation);
            return toret << "SNES Classic";
        } else {
            return toret;
        }

    } else {
        m_attachError = tr("Can't connect to the SNES Classic.");
    }
    sDebug() << "Not ready";
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

QString SNESClassicFactory::status()
{
    listDevices();
    if (device == nullptr || device->state() == ADevice::CLOSED)
    {
        return m_attachError;
    }
    else
    {
        return tr("SNES Classic ready.");
    }
}

QString SNESClassicFactory::name() const
{
    return "SNES Classic (Hakchi2CE)";
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
    if(device)
    {
        device->setMemoryLocation(ramLocation, sramLocation, romLocation);
    }
}


bool SNESClassicFactory::asyncListDevices()
{
    return false;
}
