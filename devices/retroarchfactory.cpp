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
    m_sock->write("READ_CORE_RAM 0 1");
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
        QByteArray data = m_sock->readAll();
        sDebug() << "In check for RA received" << data;
        if (data != "-1" && data != "")
        {
            retroDev = new RetroArchDevice(m_sock);
            m_devices.append(retroDev);
            toret << retroDev->name();
        }
        m_attachError = "RetroArch core selected does not support memory read";
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
        return "RetroArch device ready";
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
    deleteDevice(retroDev);
}
