
#include <QLoggingCategory>
#include <QTcpSocket>

#include "emunetworkaccessfactory.h"

Q_LOGGING_CATEGORY(log_emunwafactory, "Emu NWA Factory")
#define sDebug() qCDebug(log_emunwafactory)


// Placeholder
const quint16 emuNetworkAccessStartPort = 68200;

EmuNetworkAccessFactory::EmuNetworkAccessFactory()
{

}


QByteArray EmuNetworkAccessFactory::readSocketReturns(QTcpSocket* msocket)
{
    QByteArray toret;
    msocket->waitForReadyRead(50);
    forever {
        QByteArray data = msocket->readAll();
        sDebug() << "Reading" << data;
        if (data.isEmpty())
        {
            break;
        }
        toret += data;
        if (!msocket->waitForReadyRead(50))
        {
            break;
        }
    }
    return toret;
}


QStringList EmuNetworkAccessFactory::listDevices()
{
    QTcpSocket* socket = new QTcpSocket;
    socket->connectToHost("localhost", emuNetworkAccessStartPort);
    sDebug() << "Try to connect to localhost : " << emuNetworkAccessStartPort;

    unsigned int i = 1;
    bool connected = socket->waitForConnected(20);
    while (!connected && i != 5)
    {
        sDebug() << "Try to connect to localhost : " << emuNetworkAccessStartPort + i;
        socket->connectToHost("localhost", emuNetworkAccessStartPort + i);
        connected = socket->waitForConnected(20);
        i++;
    }
    if (!connected)
    {
        return QStringList();
    }
    /*auto emuInfos = executeHashCommand(socket, "EMU_INFOS");
    sDebug() << "Finding : " << emuInfos["name"] << emuInfos["version"];
    //auto emuState = executeHashCommand(socket, "EMU_STATUS");
    auto currentCore =  executeHashCommand(socket, "CURRENT_CORE_INFOS");
    /*if (emuState["state"] == "running" && currentCore["plateform"] == "SNES")
    {

    }
    auto currentCore =  executeHashCommand(socket, "CURRENT_CORE_INFOS");
    if (currentCore.exists("none") || currentCore["plateform"] != "SNES")
    {
        auto cores = executeHashListCommand(socket, "CORES_LIST SNES");
        sDebug() << "Core founds " << cores.size();
        if (cores.isEmpty())
        {
            //TODO handle error x)
            sDebug() << "No SNES Core found";
            return QStringList();
        }
        foreach(auto core, cores)
        {
            auto coreDetails = executeHashCommand(socket, "CORE_INFO" + core["name"]);
            sDebug() << core["name"];
        }
        newDev = new EmuNetworkAccessDevice(socket);
    }*/
    return QStringList();
}

ADevice *EmuNetworkAccessFactory::attach(QString deviceName)
{
    return nullptr;
}

bool EmuNetworkAccessFactory::deleteDevice(ADevice *)
{
    return false;
}

QString EmuNetworkAccessFactory::status()
{
    return QString();
}

QString EmuNetworkAccessFactory::name() const
{
    return "EmuNetworkAccess";
}
