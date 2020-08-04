#ifndef LUABRIDGE_H
#define LUABRIDGE_H

#include "../devicefactory.h"
#include "luabridgedevice.h"
#include <QRandomGenerator>

#include <QObject>
#include <QTcpServer>

class LuaBridge : public DeviceFactory
{
public:
    LuaBridge();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool deleteDevice(ADevice *);
    QString status();
    QString name() const;

private slots:
    void    onNewConnection();
    void    onClientDisconnected();

private:
    QStringList                 allocatedNames;
    QTcpServer*                 server;
    QList<QTcpSocket*>          clients;
    QRandomGenerator*           rng;
    quint32                     rngSeed;
    QMap<QTcpSocket*, LuaBridgeDevice*> mapSockDev;
};

#endif // LUABRIDGE_H
