#include "luabridge.h"
#include <QSettings>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_luaBridge, "LUABridge")
#define sDebug() qCDebug(log_luaBridge)
#define sInfo() qCInfo(log_luaBridge)


extern QSettings* globalSettings;

LuaBridge::LuaBridge()
{
    server = new QTcpServer(this);
    if (!globalSettings->contains("LuaBridgeRNGSeed"))
    {
        rngSeed = QRandomGenerator::global()->generate();
        globalSettings->setValue("LuaBridgeRNGSeed", rngSeed);
    } else {
        rngSeed = globalSettings->value("LuaBridgeRNGSeed").toUInt();
    }
    rng = new QRandomGenerator(rngSeed);
    connect(server, &QTcpServer::newConnection, this, &LuaBridge::onNewConnection);
    if (!server->listen(QHostAddress("127.0.0.1"), 65398))
    {
        sInfo() << "Error listening of port 65398" << server->errorString();
    } else {
        sInfo() << "Lua bridge started, listenning on 65398";
        sDebug() << "Seed is " << rngSeed;
    }

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

QString LuaBridge::status()
{
    return "Waiting for connection on 127.0.0.1:65398";
}

QString LuaBridge::name() const
{
    return "Lua Bridge";
}

void LuaBridge::onNewConnection()
{
    QTcpSocket* newclient = server->nextPendingConnection();

    QStringList names;
    names << "Cloudchaser" << "Flitter" << "Bonbon" << "Thunderlane" << "Cloud Kicker"
          << "Derpy Hooves" << "Roseluck" << "Octavia Melody" << "Dj-Pon3" << "Berrypunch";
    if (allocatedNames.size() == names.size())
    {
        newclient->close();
        return ;
    }
//#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    QString devName = names.at(rng->bounded(10));
    while (allocatedNames.contains(devName))
        devName = names.at(rng->bounded(10));
/*#else
    QString devName = names.at(qrand() % 10);
    while (allocatedNames.contains(devName))
        devName = names.at(qrand() % 10);
#endif*/
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
    sDebug() << "Lua script closed the connection" << ldev->luaName();
    ldev->closed();
    allocatedNames.removeAll(ldev->luaName());
    if (allocatedNames.isEmpty())
    {
        sDebug() << "No client connected, reseting the names";
        rng->seed(rngSeed);
    }
    deleteDevice(ldev);
}


bool LuaBridge::asyncListDevices()
{
    return false;
}
