#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>
#include <QSettings>
#include "retroarchfactory.h"

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
            m_attachError = "Can't connect to RetroArch";
            return toret;
        }
        sDebug() << "Connected";
    }
    m_sock->write("VERSION");
    m_sock->waitForReadyRead(100);
    if(m_sock->hasPendingDatagrams())
    {
        raVersion = m_sock->readAll().trimmed();
        sDebug() << raVersion;
    } else {
        m_attachError = "RetroArch - Did not get a VERSION response.";
        return toret;
    }
    m_sock->write("READ_CORE_RAM FFC0 32");
    m_sock->waitForReadyRead(100);
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
            QString gameName = "Not Available";

            if(tList.at(2) == "-1")
            {
                hasSnesMemoryMap = false;
            } else {
                unsigned char romType = static_cast<unsigned char>(QByteArray::fromHex(tList.at(23))[0]);
                unsigned char romSize = static_cast<unsigned char>(QByteArray::fromHex(tList.at(24))[0]);
                if(romType > 0 && romSize > 0)
                {
                    bool loRom = (romType & 1) == 0;
                    auto romSpeed = (romType & 0x30);
                    if(romSpeed != 0 && tList.at(2) != "00")
                    {
                        auto romName = QByteArray::fromHex(tList.mid(2, 21).join()).toStdString();
                        hasSnesMemoryMap = true;
                        hasSnesLoromMap = loRom;
                        gameName = QString::fromStdString(romName);
                    }
                }
            }

            if(!globalSettings->contains("RetroArchBlockSize"))
            {
                globalSettings->setValue("RetroArchBlockSize", "78");
            }

            auto retroBlockSize = globalSettings->value("RetroArchBlockSize").toInt();

            retroDev = new RetroArchDevice(m_sock, raVersion, gameName, retroBlockSize, hasSnesLoromMap, hasSnesMemoryMap);
            m_devices.append(retroDev);
            toret << retroDev->name();
        }
        m_attachError = QString("RetroArch %1 - Current core does not support memory read").arg(raVersion);
    } else {
        m_attachError = "RetroArch - Did not get a response to READ_CORE_RAM.";
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
        return QString("RetroArch %1 device ready").arg(raVersion);
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
