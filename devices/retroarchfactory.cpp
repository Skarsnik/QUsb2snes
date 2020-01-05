#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>
#include <QSettings>
#include <QVersionNumber>
#include "retroarchfactory.h"
#include "../rommapping/rominfo.h"

Q_LOGGING_CATEGORY(log_retroarchfact, "RA Factory")
#define sDebug() qCDebug(log_retroarchfact)

extern QSettings* globalSettings;

RetroArchFactory::RetroArchFactory()
{
    m_sock = new QUdpSocket(this);
    connect(m_sock, &QUdpSocket::disconnected, this, &RetroArchFactory::onUdpDisconnected);
    retroDev = nullptr;
}


QStringList RetroArchFactory::listDevices()
{
    m_attachError.clear();
    QStringList toret;
    if (retroDev != nullptr)
        return toret << retroDev->name();
    if (!(m_sock->state() == QUdpSocket::ConnectedState || m_sock->state() == QUdpSocket::BoundState))
    {
        sDebug() << "Trying to connect to RetroArch";

        if(!globalSettings->contains("RetroArchHost"))
        {
            globalSettings->setValue("RetroArchHost", "127.0.0.1");
        }

        auto retroarchHost = QHostAddress(globalSettings->value("RetroArchHost").toString());

        m_sock->connectToHost(retroarchHost, 55355);
        if (!m_sock->waitForConnected(50))
        {
            m_attachError = tr("Can't connect to RetroArch.");
            return toret;
        }
        sDebug() << "Connected";
    }
    if (!globalSettings->value("SkipVersionCheck", 0).toBool())
    {
        m_sock->write("VERSION");
        m_sock->waitForReadyRead(100);
        if(m_sock->hasPendingDatagrams())
        {
            raVersion = m_sock->readAll().trimmed();
            if (raVersion == "")
            {
                m_attachError = tr("RetroArch - Probably not running");
                return toret;
            }
            sDebug() << "Received RA version : " << raVersion;
        } else {
            m_attachError = tr("RetroArch - Did not get a VERSION response.");
            return toret;
        }
    }
    // Check is a game is running
    sDebug() << "Checking if something is running (read core 0 1)";
    m_sock->write("READ_CORE_RAM 0 1");
    m_sock->waitForReadyRead(100);
    QByteArray data;
    if (!m_sock->hasPendingDatagrams())
    {
        m_attachError = tr("RetroArch - No game is running or core does not support memory read.");
        return toret;
    }
    data = m_sock->readAll();
    sDebug() << "<<" << data;
    if (data == "READ_CORE_RAM 0 -1\n")
    {
        m_attachError = tr("RetroArch - No game is running or core does not support memory read.");
        return toret;
    }

    /*
     * alttp (lorom), sm (hirom), smz3 combo (exhirom)
     * Reading FFC0
     * Snes9x core
     *  loRom : all zeros
     *  hiRom : all 55
     *  exHiRom : all 55
     * BSnes-mercury
     *  loRom : correct data
     *  hiRom : correct data
     *  exHirom : incorrect data
     * --
     * Reading 40FFC0
     * Snes9x core
     *  loRom : -1
     *  hiRom : -1
     *  exHiRom : -1
     * BSnes-mercury
     *  loRom : correct data
     *  hiRom : correct data
     *  exHirom : correct data
     * */
    sDebug() << "Trying to get rom header";
    m_sock->write(QByteArray("READ_CORE_RAM 40FFC0 32"));
    m_sock->waitForReadyRead(100);
    if (m_sock->hasPendingDatagrams())
    {
        data = m_sock->readAll();
        sDebug() << "<<" << data;
        QList<QByteArray> tList = data.trimmed().split(' ');

        if(!globalSettings->contains("RetroArchBlockSize"))
        {
            globalSettings->setValue("RetroArchBlockSize", 78);
        }

        auto retroBlockSize = globalSettings->value("RetroArchBlockSize").toInt();
        if (retroBlockSize == 78 && QVersionNumber::fromString(raVersion) >= QVersionNumber::fromString("1.7.7"))
            retroBlockSize = 2000;
        // This is likely snes9x core
        if (tList.at(2) == "-1")
        {
            retroDev = new RetroArchDevice(m_sock, raVersion, retroBlockSize);
        } else {
            QString gameName;
            tList.removeFirst();
            tList.removeFirst();
            struct rom_infos* rInfos = get_rom_info(QByteArray::fromHex(tList.join()).data());
            sDebug() << rInfos->title;
            gameName = QString(rInfos->title);
            retroDev = new RetroArchDevice(m_sock, raVersion, retroBlockSize, gameName, rInfos->type);
            free(rInfos);
        }
            m_devices.append(retroDev);
            return toret << retroDev->name();
    } else {
        m_attachError = tr("RetroArch - Did not get a response to READ_CORE_RAM.");
    }
    return toret;
}

bool RetroArchFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Cleaning RetroArch device" << (retroDev != nullptr);
    Q_UNUSED(dev);
    if (retroDev == nullptr)
        return false;
    retroDev->deleteLater();
    retroDev = nullptr;
    m_devices.clear();
    return true;
}

QString RetroArchFactory::status()
{
    listDevices();
    if (retroDev != nullptr)
        return QString(tr("RetroArch %1 device ready.")).arg(raVersion);
    else
        return m_attachError;
}

QString RetroArchFactory::name() const
{
    return "RetroArch";
}

void RetroArchFactory::onUdpDisconnected()
{
    retroDev->closed();
    m_devices.clear();
    raVersion.clear();
    deleteDevice(retroDev);
}
