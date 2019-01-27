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

    explicit DeviceFactory(QObject *parent = nullptr);
    virtual QStringList listDevices() = 0;
    QList<ADevice*>     getDevices();
    virtual ADevice*    attach(QString deviceName);
    virtual bool        deleteDevice(ADevice*) = 0;
    QString             attachError() const;
    virtual QString     status() = 0;
    virtual QString     name() const = 0;
    //QList<DeviceInfo>   deviceInfos();

signals:
    void    deviceRemoved(ADevice*);


protected:
    QList<ADevice*> m_devices;
    QString         m_attachError;
};

#endif // DEVICEFACTORY_H
