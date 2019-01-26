#include "luabridge.h"

#include <QLoggingCategory>
#include <QRandomGenerator>

Q_LOGGING_CATEGORY(log_luaBridge, "LUABridge")
#define sDebug() qCDebug(log_luaBridge)

LuaBridge::LuaBridge()
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &LuaBridge::onNewConnection);
    server->listen(QHostAddress("127.0.0.1"), 65398);
    sDebug() << "Lua bridge created";
}


QStringList LuaBridge::listDevices()
{
    QStringList toret;
    foreach(ADevice* dev, m_devices)
    {
        toret << dev->name();
    }
    return toret;
}

bool LuaBridge::deleteDevice(ADevice *dev)
{
    LuaBridgeDevice* ldev = static_cast<LuaBridgeDevice*>(dev);
    m_devices.removeAll(dev);
    QTcpSocket* sock = ldev->socket();
    mapSockDev.remove(sock);
    clients.removeAll(sock);
    dev->close();
    dev->deleteLater();
    return true;
}

void LuaBridge::onNewConnection()
{
    QTcpSocket* newclient = server->nextPendingConnection();

    QStringList names;
    names << "Cloudchaser" << "Flitter" << "Bonbon" << "Thunderlane" << "Cloud Kicker"
          << "Derpy Hooves" << "Roseluck" << "Octavia Melody" << "Dj-Pon3" << "Berrypunch";
    QString devName = names.at(QRandomGenerator::global()->bounded(10));
    while (allocatedNames.contains(devName))
        devName = names.at(QRandomGenerator::global()->bounded(10));
    sDebug() << "New client connected" << devName;
    allocatedNames << devName;
    LuaBridgeDevice*    newDev = new LuaBridgeDevice(newclient, devName);
    connect(newclient, &QTcpSocket::disconnected, this, &LuaBridge::onClientDisconnected);
    m_devices.append(newDev);
    mapSockDev[newclient] = newDev;
    newclient->write(QString("SetName|%1\n").arg(devName).toLatin1());
}

void LuaBridge::onClientDisconnected()
{
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    LuaBridgeDevice* ldev = mapSockDev[sock];
    sDebug() << "Lua script closed the connection" << ldev->name();
    ldev->closed();
    allocatedNames.removeAll(ldev->name());
    deleteDevice(ldev);
}
