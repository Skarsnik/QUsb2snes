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
#include <QTcpSocket>
#include <QTime>
#include <QTimer>

#include "emunetworkaccessfactory.h"


Q_LOGGING_CATEGORY(log_emunwafactory, "Emu NWA Factory")
#define sDebug() qCDebug(log_emunwafactory)



EmuNetworkAccessFactory::EmuNetworkAccessFactory()
{
    startingPort = 0xBEEF;
    QByteArray envPort = qgetenv("NWA_PORT_RANGE");
    if (!envPort.isEmpty())
    {
        bool ok;
        unsigned short p = envPort.toUShort(&ok);
        if (ok)
            startingPort = p;
    }
    for (int i = 0; i < 5; i++)
    {
        auto piko = new EmuNWAccessClient(this);
        connect(piko, &EmuNWAccessClient::connected, this, &EmuNetworkAccessFactory::onClientConnected);
        connect(piko, &EmuNWAccessClient::connectError, this, &EmuNetworkAccessFactory::onClientConnectionError);
        connect(piko, &EmuNWAccessClient::readyRead, this, &EmuNetworkAccessFactory::onClientReadyRead);
        clientInfos[piko].checkState = DetectState::NO_CHECK;
        clientInfos[piko].device = nullptr;
        clientInfos[piko].client = piko;
        clientInfos[piko].doingAttach = false;
        clientInfos[piko].port = startingPort + i;
    }
    doingDeviceList = false;
    doingDeviceStatus = false;
    checkedCount = 0;
    devFacStatus.name = "Emu Network Access";
}


QStringList EmuNetworkAccessFactory::listDevices()
{
    return QStringList();
}

ADevice *EmuNetworkAccessFactory::attach(QString deviceName)
{
    for(auto& ci : clientInfos)
    {
        if (ci.deviceName == deviceName)
        {
            if (ci.device == nullptr)
            {
                ci.device = new EmuNetworkAccessDevice(deviceName, ci.port);
            }
            if (ci.device->state() == ADevice::BUSY)
                return ci.device;
            sDebug() << "Attach, Checking stuff";
            ci.doingAttach = true;
            ci.client->cmdEmulationStatus();
            ci.client->waitForReadyRead(100);
            auto rep = ci.client->readReply();
            ADevice* toret = nullptr;
            if (rep.isValid)
            {
                if (rep["state"] == "no_game")
                {
                    toret = ci.device;
                } else {
                    ci.client->cmdCoreCurrentInfo();
                    ci.client->waitForReadyRead(100);
                    rep = ci.client->readReply();
                    if (rep.isValid && rep["platform"].toUpper() == "SNES")
                    {
                       toret = ci.device;
                    } else {
                        //ci.lastError = "Emulator is running a no SNES game";
                        toret = nullptr;
                    }
                }
            } else {
                toret = nullptr;
            }
            ci.doingAttach = false;
            return toret;
        }
    }
    return nullptr;
}

bool EmuNetworkAccessFactory::deleteDevice(ADevice *device)
{
    sDebug() << "request for deleting device";
    QMutableMapIterator<EmuNWAccessClient*, ClientInfo> it(clientInfos);
    while (it.hasNext())
    {
        it.next();
        if (it.value().device == device)
        {
            device->deleteLater();
            it.value().device = nullptr;
            //it.value().client->deleteLater();
            it.value().deviceName.clear();
            return true;
        }
    }
    return false;
}


QString EmuNetworkAccessFactory::name() const
{
    return "EmuNetworkAccess";
}


bool EmuNetworkAccessFactory::hasAsyncListDevices()
{
    return true;
}


// FIXME If a wsclient does Attach and another DeviceList
// This will received the Attach async reply code
void    EmuNetworkAccessFactory::onClientReadyRead()
{
    EmuNWAccessClient* client = qobject_cast<EmuNWAccessClient*>(sender());
    ClientInfo &info = clientInfos[client];
    if (info.doingAttach)
        return ;
    auto rep = client->readReply();
    sDebug() << rep;
    switch (info.checkState) {
    case DetectState::DOING_NAME:
    {
        if (rep.isError) {
            checkFailed(client, Error::DeviceError::DE_EMUNWA_INCOMPATIBLE_CLIENT);
            break;
        }
        clientInfos[client].checkState = DetectState::CHECK_EMU_INFO;
        client->cmdEmulatorInfo();
        break;
    }
    case DetectState::CHECK_EMU_INFO:
    {
        if (rep.isError) {
            checkFailed(client, Error::DeviceError::DE_EMUNWA_INCOMPATIBLE_CLIENT);
            break;
        }
        auto emuInfo = rep.toMap();
        sDebug() << emuInfo;
        QString deviceName;
        if (emuInfo.contains("id"))
            deviceName = QString("%1 - %2").arg(emuInfo["name"], emuInfo["id"]);
        else
            deviceName =  QString("%1 - %2").arg(emuInfo["name"], emuInfo["version"]);
        info.deviceName = deviceName;
        info.checkState = DetectState::CHECK_CORE_LIST;
        client->cmdCoresList("SNES");
        break;
    }
    case DetectState::CHECK_CORE_LIST:
    {
        if (rep.isValid && rep.isAscii)
        {
            checkSuccess(client);
        } else {
            checkFailed(client, Error::DeviceError::DE_EMUNWA_NO_SNES_CORE);
        }
        break;
    }
    default:
        break;
    }

}

void    EmuNetworkAccessFactory::checkStatus()
{
    int cpt = 0;
    checkedCount = 0;
    QMutableMapIterator<EmuNWAccessClient*, ClientInfo> it(clientInfos);
    while (it.hasNext())
    {
        it.next();
        EmuNWAccessClient* client = it.key();
        ClientInfo& info = it.value();
        if (!client->isConnected())
        {
            info.checkState = DetectState::CHECK_CONNECTION;
            client->connectToHost("localhost", info.port);
            QTimer::singleShot(100, this, [=] {
                if (!client->isConnected())
                    checkFailed(client, Error::DeviceError::DE_EMUNWA_NO_CLIENT);
            });
        } else {
            info.checkState = DetectState::CHECK_EMU_INFO;
            client->cmdEmulatorInfo();
        }
        cpt++;
    }
}

void EmuNetworkAccessFactory::checkFailed(EmuNWAccessClient *client, Error::DeviceError err)
{
    sDebug() << "Check failed : " << err << "checked " << checkedCount + 1 << " / 5";
    clientInfos[client].checkState = DetectState::NO_CHECK;
    checkedCount++;
    if (doingDeviceStatus)
    {
        if (err != Error::DeviceError::DE_EMUNWA_NO_CLIENT)
        {
            QString piko = QString("Client %1").arg(clientInfos[client].port);
            devFacStatus.deviceNames.append(piko);
            devFacStatus.deviceStatus[piko].error = err;
            clientInfos[client].deviceName.clear();
        }
    }
    if (checkedCount == 5)
    {
        if (doingDeviceList)
        {
            doingDeviceList = false;
            clientInfos[client].deviceName.clear();
            emit devicesListDone();
        }
        if (doingDeviceStatus)
        {
            doingDeviceStatus = false;
            if (devFacStatus.deviceNames.isEmpty())
                devFacStatus.generalError = Error::DeviceFactoryError::DFE_EMUNWA_NO_CLIENT;
            emit deviceStatusDone(devFacStatus);
        }
    }
}

void EmuNetworkAccessFactory::checkSuccess(EmuNWAccessClient *client)
{
    sDebug() << "Check success : checked " << checkedCount + 1 << " / 5";
    clientInfos[client].checkState = DetectState::NO_CHECK;
    // The device is created by attach
    if (doingDeviceList)
        emit newDeviceName(clientInfos[client].deviceName);
    if (doingDeviceStatus)
    {
        QString &piko = clientInfos[client].deviceName;
        devFacStatus.deviceNames.append(piko);
        devFacStatus.deviceStatus[piko].error = Error::DeviceError::DE_NO_ERROR;
    }
    checkedCount++;
    if (checkedCount == 5)
    {
        if (doingDeviceList)
        {
            doingDeviceList = false;
            emit devicesListDone();
        }
        if (doingDeviceStatus)
        {
            doingDeviceStatus = false;
            emit deviceStatusDone(devFacStatus);
        }
    }
}

bool EmuNetworkAccessFactory::asyncListDevices()
{
    sDebug() << "Device list";
    if (doingDeviceList)
        return true;
    doingDeviceList = true;
    checkStatus();
    return true;
}

bool EmuNetworkAccessFactory::devicesStatus()
{
    sDebug() << "Devices State";
    if (doingDeviceStatus)
        return true;
    doingDeviceStatus = true;
    devFacStatus.deviceNames.clear();
    devFacStatus.deviceStatus.clear();
    devFacStatus.status = Error::DeviceFactoryStatusEnum::DFS_EMUNWA_NO_CLIENT;
    devFacStatus.generalError = Error::DeviceFactoryError::DFE_NO_ERROR;
    checkStatus();
    return true;
}

void EmuNetworkAccessFactory::onClientDisconnected()
{
    sDebug() << "Client disconnected";
    //return ; // this is kinda useless?
    EmuNWAccessClient* client = qobject_cast<EmuNWAccessClient*>(sender());
    QMutableMapIterator<EmuNWAccessClient*, ClientInfo> it(clientInfos);
    while (it.hasNext())
    {
        it.next();
        if (it.value().client == client)
        {
            sDebug()  << "Client disconnected, closing " << it.value().deviceName;
            if (it.value().device != nullptr && it.value().device->state() != ADevice::CLOSED)
                it.value().device->close();
            return;
        }
    }
}

void EmuNetworkAccessFactory::onClientConnected()
{
    EmuNWAccessClient* client = qobject_cast<EmuNWAccessClient*>(sender());
    if (clientInfos[client].checkState == DetectState::CHECK_CONNECTION)
    {

        clientInfos[client].checkState = DetectState::DOING_NAME;
        client->cmdMyNameIs("QUsb2Snes control connection");
    }
}

void EmuNetworkAccessFactory::onClientConnectionError()
{
    EmuNWAccessClient* client = qobject_cast<EmuNWAccessClient*>(sender());
    sDebug() << "Client connection error" << client->error();
    if (clientInfos[client].checkState != DetectState::NO_CHECK)
        checkFailed(client, Error::DeviceError::DE_EMUNWA_NO_CLIENT);
}
