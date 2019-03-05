#include <QEventLoop>
#include <QLoggingCategory>
#include <QTimer>
#include "retroarchfactory.h"

Q_LOGGING_CATEGORY(log_retroarchfact, "RA Factory")
#define sDebug() qCDebug(log_retroarchfact)

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
        m_sock->connectToHost(QHostAddress::LocalHost, 55355);
        if (!m_sock->waitForConnected(50))
        {
            m_attachError = "Can't connect to RetroArch";
            return toret;
        }
        sDebug() << "Connected";
    }
    m_sock->write("VERSION");
    QEventLoop loop;
    QTimer tt;
    tt.setSingleShot(true);
    tt.start(100);
    connect(&tt, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(m_sock, &QUdpSocket::readyRead, &loop, &QEventLoop::quit);
    loop.exec();
    if (tt.isActive())
    {
        tt.stop();
        raVersion = m_sock->readAll().trimmed();
    }
    m_sock->write("READ_CORE_RAM FFC0 32");
    tt.start(100);
    loop.exec();
    if (tt.isActive())
    {
        tt.stop();
        QByteArray data = m_sock->readAll();
        sDebug() << "In check for RA received" << data;
        if (data != "-1" && data != "")
        {
            /* Detect retroarch core capabilities, and SNES header info if possible */
            QList<QByteArray> tList = data.trimmed().split(' ');
            sDebug() << tList;

            bool hasSnesMemoryMap = false;
            bool hasSnesLoromMap = false;
            QString gameName = "Not Available";

            if(tList.at(2) == "-1")
            {
                hasSnesMemoryMap = false;
            } else {
                unsigned char romType = (unsigned char)QByteArray::fromHex(tList.at(23))[0];
                sDebug() << romType;
                unsigned char romSize = (unsigned char)QByteArray::fromHex(tList.at(24))[0];
                sDebug() << romSize;
                if(romType > 0 && romSize > 0)
                {
                    bool loRom = (romType & 1) == 0;
                    auto romSpeed = (romType & 0x30);
                    sDebug() << romSpeed;
                    sDebug() << "Checking rom name";
                    if(romSpeed != 0 && tList.at(2) != "00")
                    {
                        auto romName = QByteArray::fromHex(tList.mid(2, 21).join()).toStdString();
                        hasSnesMemoryMap = true;
                        hasSnesLoromMap = loRom;
                        gameName = QString::fromStdString(romName);
                    }
                }
            }

            retroDev = new RetroArchDevice(m_sock, raVersion, gameName, hasSnesLoromMap, hasSnesMemoryMap);
            m_devices.append(retroDev);
            toret << retroDev->name();
        }
        m_attachError = QString("RetroArch %1 - Current core does not support memory read").arg(raVersion);
    }
    return toret;
}

bool RetroArchFactory::deleteDevice(ADevice *dev)
{
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
