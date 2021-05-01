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

RetroArchFactory::RetroArchFactory()
{
    QString snesclassicIP;
    if (globalSettings->contains("SNESClassicIP"))
        snesclassicIP = globalSettings->value("SNESClassicIP").toString();
    else
        snesclassicIP = SNES_CLASSIC_IP;
    RAHost scHost;
    scHost.addr = QHostAddress(snesclassicIP);
    scHost.name = "SNES Classic";
    raHosts[scHost.name] = scHost;
    RAHost local;
    local.addr = QHostAddress("127.0.0.1");
    local.name = "Localhost";
    raHosts[local.name] = local;
    // Compatibility with previous version
    if (globalSettings->contains("RetroArchHost"))
    {
        QString oldEntry = globalSettings->value("RetroArchHost").toString();
        if (oldEntry == "127.0.0.1")
        {
            globalSettings->remove("RetroArchHost");
        } else {
            RAHost old;
            old.addr = QHostAddress(oldEntry);
            old.name = oldEntry;
            raHosts[oldEntry] = old;
        }
    }
    if (globalSettings->contains("RetroArchHosts"))
    {
        QString hostString = globalSettings->value("RetroArchHosts").toString();
        sDebug() << hostString;
        QStringList hosts = hostString.split(';');
        foreach(QString host, hosts) {
            QString name;
            RAHost newHost;
            if (host.contains('=')) {
                name = host.split('=').at(0);
                // TODO: refactor this blocking call to QHostInfo::fromName
                QHostInfo info = QHostInfo::fromName(host.split('=').at(1));
                QHostAddress address = info.addresses()[0];
                newHost.addr = QHostAddress(address);
            } else {
                name = host;
                // TODO: refactor this blocking call to QHostInfo::fromName
                QHostInfo info = QHostInfo::fromName(host);
                QHostAddress address = info.addresses()[0];
                newHost.addr = QHostAddress(address);
            }
            newHost.name = name;
            raHosts[name] = newHost;
            sDebug() << "Added RetroArchHost [" << newHost.name << "] with IP [" << newHost.addr << "].";
        }
    }
}


QStringList RetroArchFactory::listDevices()
{
    m_attachError.clear();
    QStringList toret;
    QMutableMapIterator<QString, RAHost> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        RAHost& host = i.value();
        sDebug() << "Checking : " << host.name;
        if (host.sock == nullptr || host.device == nullptr)
        {
            if (tryNewRetroArchHost(host))
            {
                if (host.device != nullptr)
                    toret << host.device->name();
            }
        } else {
            // We don't want to interupt something
            if (host.device->state() == ADevice::BUSY)
            {
                toret << host.device->name();
                continue;
            }
            disconnect(host.sock, &QUdpSocket::readyRead, host.device, nullptr);
            RetroArchInfos info = RetroArchDevice::getRetroArchInfos(host.sock);
            if (info.error.isEmpty())
                toret << host.device->name();
            connect(host.sock, &QUdpSocket::readyRead, host.device, &RetroArchDevice::onUdpReadyRead);
        }
     }
    return toret;
}

bool RetroArchFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Cleaning RetroArch device :" << dev->name();
    RetroArchDevice* raDev = static_cast<RetroArchDevice*>(dev);
    QMutableMapIterator<QString, RAHost> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        if (i.value().device == raDev)
        {
            i.value().device = nullptr;
        }
    }
    raDev->deleteLater();
    return true;
}

QString RetroArchFactory::status()
{
    QString status;
    listDevices();
    QMapIterator<QString, RAHost> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        const RAHost& host = i.value();
        status += QString("RetroArch %1 : %2").arg(host.name).arg(host.status);
        if (i.hasNext())
            status += " | ";
    }
    sDebug() << status;
    return status;
}

QString RetroArchFactory::name() const
{
    return "RetroArch";
}

ADevice *RetroArchFactory::attach(QString deviceName)
{
    m_attachError.clear();
    QMapIterator<QString, RAHost> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        if (i.value().device != nullptr && i.value().device->name() == deviceName)
            return i.value().device;
    }
    return nullptr;
}

void RetroArchFactory::onUdpDisconnected()
{

}

bool RetroArchFactory::tryNewRetroArchHost(RAHost& host)
{
    QUdpSocket* sock = nullptr;
    sDebug() << "Trying to connect to New RetroArch" << host.name << host.addr;
    if (host.sock == nullptr || !host.sock->isOpen())
    {
        if (host.sock == nullptr)
            host.sock = new QUdpSocket(this);
        host.sock->connectToHost(host.addr, host.port);
        if (!host.sock->waitForConnected(200))
        {
            host.status = tr("Can't connect to RetroArch.");
            m_attachError = host.status;
            host.sock->deleteLater();
            host.sock = nullptr;
            return false;
        }
    }
    sock = host.sock;
    sDebug() << "Connected, aka can create the connection";
    sock->write("VERSION");
    sock->waitForReadyRead(200);
    if(sock->hasPendingDatagrams())
    {
        QString raVersion = sock->readAll().trimmed();
        if (raVersion == "")
        {
            m_attachError = tr("Probably not running");
            host.status = m_attachError;
            return false;
        }
        sDebug() << "Received RA version : " << raVersion;
    } else {
        m_attachError = tr("Could not get the version.");
        host.status = m_attachError;
        return false;
    }
    RetroArchInfos info = RetroArchDevice::getRetroArchInfos(sock);
    if (!info.error.isEmpty())
    {
        m_attachError = info.error;
        host.status = info.error;
        return false;
    }
    RetroArchDevice* newDev = new RetroArchDevice(sock, info, host.name);
    host.device = newDev;
    m_devices << newDev;
    host.status = QString("RetroArch %1, running %2").arg(info.version).arg(info.gameName);
    return true;
}


bool RetroArchFactory::asyncListDevices()
{
    return false;
}
