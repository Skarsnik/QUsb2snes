#ifndef RETROARCHFACTORY_H
#define RETROARCHFACTORY_H

#include <QObject>
#include <QUdpSocket>
#include "../devicefactory.h"
#include "retroarchdevice.h"


class RetroArchFactory : public DeviceFactory
{
    Q_OBJECT
    struct RAHost {
        RAHost() {
            sock = nullptr;
            port = 55355;
            device = nullptr;
        }
        QString             name;
        QHostAddress        addr;
        QUdpSocket*         sock;
        quint16             port;
        QString             status;
        RetroArchDevice*    device;
    };

public:
    RetroArchFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool deleteDevice(ADevice *dev);
    QString status();
    QString name() const;
    ADevice* attach(QString deviceName);


private slots:
    void    onUdpDisconnected();

private:
    QMap<QString, RAHost>           raHosts;

    bool            checkRetroArchHost(RAHost& host);

    bool            tryNewRetroArchHost(RAHost& host);
};

#endif // RETROARCHFACTORY_H
