
#include <QLoggingCategory>
#include <QTcpSocket>
#include <QTimer>

#include "emunetworkaccessfactory.h"

Q_LOGGING_CATEGORY(log_emunwafactory, "Emu NWA Factory")
#define sDebug() qCDebug(log_emunwafactory)


// Placeholder
const quint16 emuNetworkAccessStartPort = 68200;

EmuNetworkAccessFactory::EmuNetworkAccessFactory()
{

}


QStringList EmuNetworkAccessFactory::listDevices()
{
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


bool EmuNetworkAccessFactory::hasAsyncListDevices()
{
    return true;
}

bool EmuNetworkAccessFactory::asyncListDevices()
{

    return true;
}
