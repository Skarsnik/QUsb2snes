#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_remoteusb2snes, "Remote factory")
#define sDebug() qCDebug(log_remoteusb2snes)
#define sInfo() qCInfo(log_remoteusb2snes)

#include "remoteusb2sneswfactory.h"
#include "remoteusb2sneswdevice.h"
#include "settings.hpp"

extern Settings*    mySettings;


RemoteUsb2SnesWFactory::RemoteUsb2SnesWFactory(QObject *parent)
    : DeviceFactory{parent}
{
    devFacStatus.name = name();
}

QStringList RemoteUsb2SnesWFactory::listDevices()
{
    return QStringList();
}

QUrl RemoteUsb2SnesWFactory::remoteUrl()
{
    return QUrl("ws://" + mySettings->value<Settings::RemoteHost>() + ":" + QString::number(USB2SnesWS::defaultPort));
}

ADevice* RemoteUsb2SnesWFactory::attach(QString deviceName)
{
    if (remote == nullptr)
        createRemote();
    sDebug() << "Attach";
    if (remote->state() != Usb2Snes::Ready)
    {
        checkStatus();
        return nullptr;
    }
    QEventLoop loop;
    doingAttach = true;
    connect(remote, &Usb2Snes::deviceListDone, &loop, &QEventLoop::quit);
    connect(remote, &Usb2Snes::disconnected, &loop, &QEventLoop::quit);
    remote->deviceList();
    loop.exec();
    doingAttach = false;
    if (remote->state() == Usb2Snes::None || deviceList.isEmpty())
        return nullptr;
    for (QString dev : deviceList)
    {
        if (mapLocalNamesToRemoteNames.key(dev) == deviceName)
        {
            RemoteUsb2snesWDevice* newDev = new RemoteUsb2snesWDevice(mapLocalNamesToRemoteNames[deviceName]);
            newDev->createWebsocket(remoteUrl());
            return newDev;
        }
    }
    return nullptr;
}

bool RemoteUsb2SnesWFactory::deleteDevice(ADevice *)
{
    return true;
}

QString RemoteUsb2SnesWFactory::name() const
{
    return "Remote Usb2snes wserver";
}

bool RemoteUsb2SnesWFactory::hasAsyncListDevices()
{
    return true;
}

bool RemoteUsb2SnesWFactory::asyncListDevices()
{
    sDebug() << "Device list";
    if (doingDeviceList)
        return true;
    doingDeviceList = true;
    checkStatus();
    return true;
}

void    RemoteUsb2SnesWFactory::checkFailed(Error::DeviceFactoryError err)
{
    if (doingDeviceList)
    {
        doingDeviceList = false;
        emit devicesListDone();
    }
    if (doingDeviceStatus)
    {
        devFacStatus.generalError = err;
        doingDeviceStatus = false;
        emit deviceStatusDone(devFacStatus);
    }
}

bool RemoteUsb2SnesWFactory::devicesStatus()
{
    sDebug() << "Devices Status";
    if (doingDeviceStatus)
        return true;
    doingDeviceStatus = true;
    devFacStatus.deviceNames.clear();
    checkStatus();
    return true;
}

void    RemoteUsb2SnesWFactory::createRemote()
{
    sDebug() << "Creating the client";
    remote = new Usb2Snes(false);
    connect(remote, &Usb2Snes::connected, this, [=] {
        sDebug() << "Usb2snes connected";
        remote->setAppName("QUsb2Snes Remote access");
        remote->appVersion();
    });
    connect(remote, &Usb2Snes::appVersionDone, this, [=](QString name)
            {
                remoteName = name;
                remote->deviceList();
            });
    connect(remote, &Usb2Snes::deviceListDone, this, [=](QStringList devs)
            {
                if (doingAttach)
                {
                    deviceList = devs;
                }
                if (devs.isEmpty())
                {
                    checkFailed(Error::DeviceFactoryError::DFE_REMOTE_NO_DEVICE);
                } else {
                    mapLocalNamesToRemoteNames.clear();
                    for (const auto& dev : devs)
                    {
                        mapLocalNamesToRemoteNames[remoteName + " - " + dev] = dev;
                    }
                    if (doingDeviceList) {
                        for (const auto& dev : devs)
                        {
                            emit newDeviceName(remoteName + " - " + dev);
                        }
                        doingDeviceList = false;
                        emit devicesListDone();
                    }
                    if (doingDeviceStatus)
                    {
                        devFacStatus.status = Error::DeviceFactoryStatusEnum::DFS_REMOTE_READY;
                        for (const auto &dev : devs)
                        {
                            devFacStatus.deviceNames.append(remoteName + " - " + dev);
                            //devFacStatus.deviceStatus[remoteName + " - " + dev].state;
                            devFacStatus.deviceStatus[remoteName + " - " + dev].error = Error::DeviceError::DE_NO_ERROR;
                        }
                        doingDeviceStatus = false;
                        emit deviceStatusDone(devFacStatus);
                    }
                }
            });
}

void    RemoteUsb2SnesWFactory::checkStatus()
{
    if (remote == nullptr)
    {
        createRemote();
    }
    sDebug() << "Usb2snes state : " << remote->state();
    if (remote->state() == Usb2Snes::Ready)
    {
        sDebug() << "Connected and free";
        remote->deviceList();
    } else {
        sDebug() << "Attempt to connect to remote";
        remote->connectToHost(mySettings->value<Settings::RemoteHost>(), USB2SnesWS::defaultPort);
        QTimer::singleShot(100, this, [=] {
            if (remote->state() == Usb2Snes::None)
            {
                sDebug() << "Timeout request to connect";
                remote->disconnectFromHost();
                checkFailed(Error::DeviceFactoryError::DFE_REMOTE_NO_REMOTE);
            }
        });
    }
}
