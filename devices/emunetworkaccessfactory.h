#ifndef EMUNETWORKACCESSFACTORY_H
#define EMUNETWORKACCESSFACTORY_H

#include <QObject>
#include "../devicefactory.h"
#include "emunetworkaccessdevice.h"
#include "emunwaccessclient.h"

class EmuNetworkAccessFactory : public DeviceFactory
{
    Q_OBJECT
private:
    struct ClientInfo {
        EmuNWAccessClient*      client;
        EmuNetworkAccessDevice* device;
        QString                 lastError;
        QString                 deviceName;
    };

public:
    EmuNetworkAccessFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString status();
    QString name() const;
    bool hasAsyncListDevices();
    bool asyncListDevices();

    QList<ClientInfo>   clientInfos;

 private slots:
    void    onClientDisconnected();
};

#endif // EMUNETWORKACCESSFACTORY_H
