#ifndef REMOTEUSB2SNESWFACTORY_H
#define REMOTEUSB2SNESWFACTORY_H

#include <devicefactory.h>
#include "client/usb2snesclient.h"

class RemoteUsb2SnesWFactory : public DeviceFactory
{
    Q_OBJECT
public:
    explicit RemoteUsb2SnesWFactory(QObject *parent = nullptr);

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString name() const;
    bool hasAsyncListDevices();
    bool asyncListDevices();
    bool devicesStatus();
private:
    Usb2Snes*   remote = nullptr;
    QString     remoteName;
    bool        doingDeviceList = false;
    bool        doingDeviceStatus = false;
    bool        doingAttach = false;
    DeviceFactoryStatus  devFacStatus;
    void        checkFailed(Error::DeviceFactoryError err);
    void        checkStatus();
    QUrl        remoteUrl;
    void        createRemote();
    QStringList deviceList;

};

#endif // REMOTEUSB2SNESWFACTORY_H
