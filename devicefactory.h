#ifndef DEVICEFACTORY_H
#define DEVICEFACTORY_H

#include <QObject>
#include "adevice.h"

class DeviceFactory : public QObject
{
    Q_OBJECT
public:
    /*
    struct DeviceInfo {
        ADevice *device;

    };*/
    enum DeviceFactoryState {
        NONE,
        BUSYDEVICELIST,
        BUSYATTACH
    };

    explicit DeviceFactory(QObject *parent = nullptr);
    virtual QStringList listDevices() = 0;
    QList<ADevice*>     getDevices();
    virtual ADevice*    attach(QString deviceName);
    virtual bool        deleteDevice(ADevice*) = 0;
    QString             attachError() const;
    virtual QString     status() = 0;
    virtual QString     name() const = 0;
    virtual bool        hasAsyncListDevices();
    virtual bool        asyncListDevices() = 0;
    virtual QStringList getDevicesName() const;
    //QList<DeviceInfo>   deviceInfos();

signals:
    void    deviceRemoved(ADevice*);
    void    newDeviceName(QString name);
    void    devicesListDone();


protected:
    QList<ADevice*> m_devices;
    QString         m_attachError;
    DeviceFactoryState  m_state;
    QStringList     m_listDeviceNames;
};

#endif // DEVICEFACTORY_H
