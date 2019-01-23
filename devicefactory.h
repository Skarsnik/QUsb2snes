#ifndef DEVICEFACTORY_H
#define DEVICEFACTORY_H

#include <QObject>
#include "adevice.h"

class DeviceFactory : public QObject
{
    Q_OBJECT
public:
    explicit DeviceFactory(QObject *parent = nullptr);
    virtual QStringList listDevices() = 0;
    QList<ADevice*>     getDevices();
    virtual ADevice*    attach(QString deviceName);
    virtual void        deleteDevice(ADevice*) = 0;
    QString             attachError() const;

signals:
    void    deviceRemoved(ADevice*);


protected:
    QList<ADevice*> m_devices;
    QString         m_attachError;
};

#endif // DEVICEFACTORY_H
