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

#include "luabridge.h"
#include <QSettings>

#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(log_luaBridge, "LUABridge")
#define sDebug() qCDebug(log_luaBridge)
#define sInfo() qCInfo(log_luaBridge)


extern QSettings* globalSettings;

LuaBridge::LuaBridge()
{
    server = new QTcpServer(this);
    if (!globalSettings->contains("LuaBridgeRNGSeed"))
    {
        rngSeed = QRandomGenerator::global()->generate();
        globalSettings->setValue("LuaBridgeRNGSeed", rngSeed);
    } else {
        rngSeed = globalSettings->value("LuaBridgeRNGSeed").toUInt();
    }
    rng = new QRandomGenerator(rngSeed);
    connect(server, &QTcpServer::newConnection, this, &LuaBridge::onNewConnection);
    if (!server->listen(QHostAddress("127.0.0.1"), 65398))
    {
        sInfo() << "Error listening of port 65398" << server->errorString();
    } else {
        sInfo() << "Lua bridge started, listenning on 65398";
        sDebug() << "Seed is " << rngSeed;
    }

}


QStringList LuaBridge::listDevices()
{
    sDebug() << "List device";
    QStringList toret;
    foreach(ADevice* dev, m_devices)
    {
        toret << dev->name();
    }
    return toret;
}

bool LuaBridge::deleteDevice(ADevice *dev)
{
    LuaBridgeDevice* ldev = static_cast<LuaBridgeDevice*>(dev);
    m_devices.removeAll(dev);
    QTcpSocket* sock = ldev->socket();
    mapSockDev.remove(sock);
    clients.removeAll(sock);
    dev->close();
    dev->deleteLater();
    return true;
}

QString LuaBridge::name() const
{
    return "Lua Bridge";
}

void LuaBridge::onNewConnection()
{
    QTcpSocket* newclient = server->nextPendingConnection();

    QStringList names;
    names << "Cloudchaser" << "Flitter" << "Bonbon" << "Thunderlane" << "Cloud Kicker"
          << "Derpy Hooves" << "Roseluck" << "Octavia Melody" << "Dj-Pon3" << "Berrypunch";
    if (allocatedNames.size() == names.size())
    {
        newclient->close();
        return ;
    }
//#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    QString devName = names.at(rng->bounded(10));
    while (allocatedNames.contains(devName))
        devName = names.at(rng->bounded(10));
/*#else
    QString devName = names.at(qrand() % 10);
    while (allocatedNames.contains(devName))
        devName = names.at(qrand() % 10);
#endif*/
    sDebug() << "New client connected" << devName;
    allocatedNames << devName;
    LuaBridgeDevice*    newDev = new LuaBridgeDevice(newclient, devName);
    connect(newclient, &QTcpSocket::disconnected, this, &LuaBridge::onClientDisconnected);
    m_devices.append(newDev);
    mapSockDev[newclient] = newDev;
    newclient->write(QString("SetName|%1\n").arg(devName).toLatin1());
}

void LuaBridge::onClientDisconnected()
{
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    LuaBridgeDevice* ldev = mapSockDev[sock];
    sDebug() << "Lua script closed the connection" << ldev->luaName();
    emit ldev->closed();
    allocatedNames.removeAll(ldev->luaName());
    if (allocatedNames.isEmpty())
    {
        sDebug() << "No client connected, reseting the names";
        rng->seed(rngSeed);
    }
    deleteDevice(ldev);
}


bool LuaBridge::asyncListDevices()
{
    QTimer::singleShot(0, this, [=] {
        foreach(ADevice* dev, m_devices)
        {
            emit newDeviceName(dev->name());
        }
        emit devicesListDone();
    });
    return true;
}


bool LuaBridge::devicesStatus()
{
    sDebug() << "Lua status";
    QTimer::singleShot(0, this, [=] {
        sDebug() << "Piko";
        DeviceFactoryStatus status;
        status.status = Error::DeviceFactoryStatusEnum::DFS_LUA_LISTENNING;
        status.generalError = Error::DeviceFactoryError::DFE_LUA_CANT_LISTEN;
        status.name = "Lua Bridge";
        status.deviceNames.clear();
        status.deviceStatus.clear();
        if (server->isListening())
        {
            status.status = Error::DeviceFactoryStatusEnum::DFS_LUA_LISTENNING;
            status.generalError = Error::DeviceFactoryError::DFE_NO_ERROR;
        }
        if (!mapSockDev.isEmpty())
        {
            QMapIterator<QTcpSocket*, LuaBridgeDevice*> i(mapSockDev);
            while (i.hasNext()) {
                i.next();
                LuaBridgeDevice* dev = i.value();
                status.deviceNames.append(dev->name());
                status.deviceStatus[dev->name()].state = dev->state();
                status.deviceStatus[dev->name()].error = Error::DeviceError::DE_NO_ERROR;
            }
        }
        sDebug() << "Lua done";
        emit deviceStatusDone(status);
    });
    return true;
}


bool LuaBridge::hasAsyncListDevices()
{
    return true;
}
