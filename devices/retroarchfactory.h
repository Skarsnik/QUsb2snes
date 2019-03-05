#ifndef RETROARCHFACTORY_H
#define RETROARCHFACTORY_H

#include <QObject>
#include <QUdpSocket>
#include "../devicefactory.h"
#include "retroarchdevice.h"


class RetroArchFactory : public DeviceFactory
{
public:
    RetroArchFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool deleteDevice(ADevice *dev);
    QString status();
    QString name() const;


private slots:
    void    onUdpDisconnected();
private:
    QUdpSocket*      m_sock;
    QString          raVersion;
    RetroArchDevice* retroDev;
};

#endif // RETROARCHFACTORY_H
