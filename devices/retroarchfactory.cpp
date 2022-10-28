/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet, Greg J Arnold.
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

#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>
#include <QSettings>
#include <QVersionNumber>
#include <QHostInfo>
#include "retroarchfactory.h"
#include "../rommapping/rominfo.h"

#define SNES_CLASSIC_IP "169.254.13.37"

Q_LOGGING_CATEGORY(log_retroarchfact, "RA Factory")
#define sDebug() qCDebug(log_retroarchfact)

extern QSettings* globalSettings;

void    RetroArchFactory::addHost(RetroArchHost* host)
{
    raHosts[host->name()].host = host;
    raHosts[host->name()].error = false;
    connect(host, &RetroArchHost::infoDone, this, &RetroArchFactory::onRaHostInfosDone);
    connect(host, &RetroArchHost::infoFailed, this, &RetroArchFactory::onRaHostgetInfosFailed);
    connect(host, &RetroArchHost::errorOccured, this, &RetroArchFactory::onRaHostErrorOccured);
    sDebug() << "Added new RetroArch host: " << host->name();
    raHosts[host->name()].name = host->name();
    doingListDevices = false;
    doingDevicesStatus = false;
}

RetroArchFactory::RetroArchFactory()
{
    hostChecked = -1;
    hostCheckCount = -1;

    QString snesclassicIP;
    if (globalSettings->contains("SNESClassicIP"))
        snesclassicIP = globalSettings->value("SNESClassicIP").toString();
    else
        snesclassicIP = SNES_CLASSIC_IP;
    RetroArchHost* scHost = new RetroArchHost("Snes Classic");
    scHost->setHostAddress(QHostAddress(snesclassicIP));
    addHost(scHost);
    RetroArchHost* local = new RetroArchHost("Localhost");
    local->setHostAddress(QHostAddress("127.0.0.1"));
    addHost(local);
    // Compatibility with previous version
    if (globalSettings->contains("RetroArchHost"))
    {
        QString oldEntry = globalSettings->value("RetroArchHost").toString();
        if (oldEntry == "127.0.0.1")
        {
            globalSettings->remove("RetroArchHost");
        } else {
            RetroArchHost* old = new RetroArchHost(oldEntry);
            QHostInfo::lookupHost(oldEntry, this, [old](QHostInfo hinfo){
                old->setHostAddress(hinfo.addresses().at(0));
            });
           addHost(old);
        }
    }
    if (globalSettings->contains("RetroArchHosts"))
    {
        QString hostString = globalSettings->value("RetroArchHosts").toString();
        sDebug() << hostString;
        QStringList hosts = hostString.split(';');
        foreach(QString host, hosts) {
            RetroArchHost* newHost;
            if (host.contains('=')) {
                newHost = new RetroArchHost(host.split('=').at(0));
                QHostInfo::lookupHost(host.split('=').at(1), this, [newHost](QHostInfo hinfo){
                    sDebug() << "RetroArchHost [" << newHost->name() << "] get IP [" << newHost->address() << "].";
                    newHost->setHostAddress(hinfo.addresses().at(0));
                });
            } else {
                newHost = new RetroArchHost(host);
                QHostInfo::lookupHost(host, this, [newHost](QHostInfo hinfo){
                    sDebug() << "RetroArchHost [" << newHost->name() << "] get IP [" << newHost->address() << "].";
                    newHost->setHostAddress(hinfo.addresses().at(0));
                });
            }
            addHost(newHost);
        }
    }
}


QStringList RetroArchFactory::listDevices()
{
    m_attachError.clear();
    QStringList toret;
    return toret;
}

bool RetroArchFactory::hasAsyncListDevices()
{
    return true;
}


void    RetroArchFactory::checkDevices()
{
    QMutableMapIterator<QString, HostData> it(raHosts);
    hostCheckCount = 0;
    hostChecked = 0;

    while (it.hasNext())
    {
        it.next();
        HostData& data = it.value();
        // Device exist and it's doing something, yay
        if (data.device != nullptr && data.device->state() == ADevice::BUSY)
        {
            if (doingListDevices)
                emit newDeviceName(data.device->name());
            if (doingDevicesStatus)
            {
                m_status.deviceNames.append(data.device->name());
                m_status.deviceStatus[data.device->name()].state = ADevice::BUSY;
                m_status.deviceStatus[data.device->name()].error = Error::DeviceError::DE_NO_ERROR;
            }
            continue;
        } else {
            hostCheckCount++;
            data.error = false;
            data.reqId = data.host->getInfos();
            sDebug() << data.name << "Info id" << data.reqId;
        }
    }
}

bool RetroArchFactory::asyncListDevices()
{
    doingListDevices = true;
    doingDevicesStatus = false;
    checkDevices();
    return true;
}



bool RetroArchFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Cleaning RetroArch device :" << dev->name();
    RetroArchDevice* raDev = static_cast<RetroArchDevice*>(dev);
    QMutableMapIterator<QString, HostData> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        if (i.value().device == raDev)
        {
            i.value().device = nullptr;
            break;
        }
    }
    raDev->deleteLater();
    return true;
}

QString RetroArchFactory::name() const
{
    return "RetroArch";
}

ADevice *RetroArchFactory::attach(QString deviceName)
{
    m_attachError.clear();
    QMapIterator<QString, HostData> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        if (i.value().device != nullptr && i.value().device->name() == deviceName)
            return i.value().device;
    }
    return nullptr;
}

bool RetroArchFactory::devicesStatus()
{
    doingDevicesStatus = true;
    m_status.generalError = Error::DeviceFactoryError::DFE_NO_ERROR;
    m_status.status = Error::DeviceFactoryStatusEnum::DFS_RETROARCH_NO_RA;
    m_status.name = "RetroArch";
    m_status.deviceNames.clear();
    m_status.deviceStatus.clear();
    checkDevices();
    return true;
}


void RetroArchFactory::onRaHostInfosDone(qint64 id)
{
    sDebug() << "Info done" << id;
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    if (raHosts[host->name()].reqId != id)
        return ;
    HostData& data = raHosts[host->name()];
    data.error = false;
    if (data.device == nullptr)
        data.device = new RetroArchDevice(host);
    if (doingListDevices)
        emit newDeviceName(data.device->name());
    if (doingDevicesStatus)
    {
        m_status.deviceNames.append(data.device->name());
        m_status.deviceStatus[data.device->name()].state = ADevice::READY;
        m_status.deviceStatus[data.device->name()].error = Error::DeviceError::DE_NO_ERROR;
    }
    checkInfoDone();
}

void RetroArchFactory::onRaHostgetInfosFailed(qint64 id)
{    
    sDebug() << "Info failed" << id;
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    if (raHosts[host->name()].reqId != id)
        return ;
    if (doingDevicesStatus)
    {
        const QString& devName = raHosts[host->name()].name;
        m_status.deviceNames.append(devName);
        m_status.deviceStatus[devName].state = ADevice::CLOSED;
        m_status.deviceStatus[devName].error = Error::DeviceError::DE_RETROARCH_INFO_FAILED;
        m_status.deviceStatus[devName].overridedErrorString = host->lastInfoError();
    }
    raHosts[host->name()].error = true;
    sDebug() << "Info failed for" << host->name();
    checkInfoDone();
}

void RetroArchFactory::onRaHostErrorOccured(QAbstractSocket::SocketError err)
{
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    sDebug() << host->name() << err;
    raHosts[host->name()].error = true;
    // Doing info
    if (hostChecked != -1)
    {
        if (doingDevicesStatus)
        {
            const QString& devName = raHosts[host->name()].name;
            sDebug() << devName;
            m_status.deviceNames.append(devName);
            m_status.deviceStatus[devName].state = ADevice::CLOSED;
            if (err == QAbstractSocket::NetworkError)
                m_status.deviceStatus[devName].error = Error::DeviceError::DE_RETROARCH_UNREACHABLE;
            if (err == QAbstractSocket::ConnectionRefusedError)
                m_status.deviceStatus[devName].error = Error::DeviceError::DE_RETROARCH_NO_CONNECTION;
        }
        checkInfoDone();
    }
}

void RetroArchFactory::checkInfoDone()
{
    hostChecked++;
    if (hostChecked == hostCheckCount)
    {
        hostChecked = -1;
        hostCheckCount = -1;
        if (doingListDevices)
        {
            emit devicesListDone();
            doingListDevices = false;
        }
        if (doingDevicesStatus)
        {
            emit deviceStatusDone(m_status);
            doingDevicesStatus = false;
        }
    }
}

