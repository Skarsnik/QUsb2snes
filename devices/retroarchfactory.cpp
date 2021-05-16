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
    connect(host, &RetroArchHost::infoDone, this, &RetroArchFactory::onRaHostInfosDone);
    connect(host, &RetroArchHost::infoFailed, this, &RetroArchFactory::onRaHostgetInfosFailed);
    connect(host, &RetroArchHost::errorOccured, this, &RetroArchFactory::onRaHostErrorOccured);
    connect(host, &RetroArchHost::connected, this, &RetroArchFactory::onRaHostConnected);
    sDebug() << "Added new RetroArch host: " << host->name();
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
                old->setHostAddress(hinfo.addresses()[0]);
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
            QString name;
            RetroArchHost* newHost;
            if (host.contains('=')) {
                newHost = new RetroArchHost(host.split('=').at(0));
                QHostInfo::lookupHost(host.split('=').at(1), this, [newHost](QHostInfo hinfo){
                    sDebug() << "RetroArchHost [" << newHost->name() << "] get IP [" << newHost->address() << "].";
                    newHost->setHostAddress(hinfo.addresses()[0]);
                });
            } else {
                newHost = new RetroArchHost(host);
                QHostInfo::lookupHost(host, this, [newHost](QHostInfo hinfo){
                    sDebug() << "RetroArchHost [" << newHost->name() << "] get IP [" << newHost->address() << "].";
                    newHost->setHostAddress(hinfo.addresses()[0]);
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
    /*QMutableMapIterator<QString, RAHost> i(raHosts);
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
     }*/
    return toret;
}

bool RetroArchFactory::hasAsyncListDevices()
{
    return true;
}


bool RetroArchFactory::asyncListDevices()
{
    QMutableMapIterator<QString, HostData> it(raHosts);
    hostCheckCount = 0;
    hostChecked = 0;
    while (it.hasNext())
    {
        it.next();
        QString name = it.key();
        HostData& data = it.value();
        // Device exist and it's doing something, yay
        if (data.device != nullptr && data.device->state() == ADevice::BUSY)
        {
            emit newDeviceName(data.device->name());
            continue;
        } else {
            hostCheckCount++;
            if (!data.host->isConnected())
            {
                sDebug() << "Trying to connect " << data.host->name();
                data.host->connectToHost();
            } else {
                data.reqId = data.host->getInfos();
            }
        }
    }
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

QString RetroArchFactory::status()
{
    QString status;
    listDevices();
    QMapIterator<QString, HostData> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        const HostData& host = i.value();
        status += QString("RetroArch %1 ").arg(host.name);
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
    QMapIterator<QString, HostData> i(raHosts);
    while (i.hasNext())
    {
        i.next();
        if (i.value().device != nullptr && i.value().device->name() == deviceName)
            return i.value().device;
    }
    return nullptr;
}


void RetroArchFactory::onRaHostInfosDone(qint64 id)
{
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    if (raHosts[host->name()].reqId != id)
        return ;
    HostData& data = raHosts[host->name()];
    if (data.device == nullptr)
        data.device = new RetroArchDevice(host);
    emit newDeviceName(data.device->name());
    checkInfoDone();
}

void RetroArchFactory::onRaHostgetInfosFailed(qint64 id)
{    
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    if (raHosts[host->name()].reqId != id)
        return ;
    sDebug() << "Info failed for" << host->name();
    checkInfoDone();
}

void RetroArchFactory::onRaHostErrorOccured(QAbstractSocket::SocketError err)
{
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    sDebug() << host->name() << err;
    // Doing info
    if (hostChecked != -1)
    {
        disconnect(host, &RetroArchHost::connected, this, nullptr);
        checkInfoDone();
    }
}

void RetroArchFactory::onRaHostConnected()
{
    RetroArchHost*  host = qobject_cast<RetroArchHost*>(sender());
    sDebug() << host->name() << "Connected";
    if (hostChecked != -1)
    {
         QMutableMapIterator<QString, HostData> it(raHosts);
         while (it.hasNext())
         {
             it.next();
             if (it.value().host == host)
             {
                 it.value().reqId = host->getInfos();
             }
         }
    }
}

void RetroArchFactory::checkInfoDone()
{
    hostChecked++;
    if (hostChecked == hostCheckCount)
    {
        hostChecked = -1;
        hostCheckCount = -1;
        emit devicesListDone();
    }
}

