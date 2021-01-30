#ifndef EMUNETWORKACCESSFACTORY_H
#define EMUNETWORKACCESSFACTORY_H

#include <QObject>
#include "../devicefactory.h"

class EmuNetworkAccessFactory : public DeviceFactory
{
public:
    EmuNetworkAccessFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString status();
    QString name() const;
private:
    QByteArray readSocketReturns(QTcpSocket *msocket);
};

#endif // EMUNETWORKACCESSFACTORY_H
