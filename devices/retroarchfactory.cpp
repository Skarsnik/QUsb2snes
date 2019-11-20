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
            sDebug() << "Received RA version : " << raVersion;
        } else {
            m_attachError = tr("RetroArch - Did not get a VERSION response.");
            return toret;
        }
    }
    // Should be loRom rom address
    auto loRomHeaderAddr = QByteArray::number(0x7FC0 + (0x8000 * ((0x7FC0 + 0x8000) / 0x8000)));
    // but let's check HiROM
    m_sock->write(QByteArray("READ_CORE_RAM 40FFC0 32"));// + loRomHeaderAddr.constData());
    m_sock->waitForReadyRead(100);
    //TODO: it's hard to tell what we can read/write, RA UDP support is a nightmare :(
    if (m_sock->hasPendingDatagrams())
    {
        QByteArray data = m_sock->readAll();
        if (data != "-1" && data != "")
        {

            sDebug() << data;
            /* Detect retroarch core capabilities, and SNES header info if possible */
            QList<QByteArray> tList = data.trimmed().split(' ');
            bool hasSnesMemoryMap = false;
            bool hasSnesLoromMap = false;
            QString gameName;

            if(tList.at(2) == "-1")
            {
                hasSnesMemoryMap = false;
            } else {
                tList.removeFirst();
                tList.removeFirst();
                struct rom_infos* rInfos = get_rom_info(QByteArray::fromHex(tList.join()).data());
                sDebug() << rInfos->title;
                gameName = QString(rInfos->title);
                hasSnesLoromMap = !rom_info_make_sense(rInfos, HiROM);
                hasSnesMemoryMap = true;
                free(rInfos);
            }

            if(!globalSettings->contains("RetroArchBlockSize"))
            {
                globalSettings->setValue("RetroArchBlockSize", 78);
            }

            auto retroBlockSize = globalSettings->value("RetroArchBlockSize").toInt();
            if (retroBlockSize == 78 && QVersionNumber::fromString(raVersion) >= QVersionNumber::fromString("1.7.7"))
                retroBlockSize = 2000;
            retroDev = new RetroArchDevice(m_sock, raVersion, gameName, retroBlockSize, hasSnesMemoryMap, hasSnesLoromMap);
            m_devices.append(retroDev);
            return toret << retroDev->name();
        }
        m_attachError = QString(tr("RetroArch %1 - Current core does not support memory read.")).arg(raVersion);
    } else {
        m_attachError = tr("RetroArch - Did not get a response to READ_CORE_RAM.");
    }
    return toret;
}

bool RetroArchFactory::deleteDevice(ADevice *dev)
{
    Q_UNUSED(dev);
    if (retroDev == nullptr)
        return false;
    retroDev->deleteLater();
    retroDev = nullptr;
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
