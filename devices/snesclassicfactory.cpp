#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_snesclassicfact, "SNESClassic Factory")
#define sDebug() qCDebug(log_snesclassicfact)

#define SNES_CLASSIC_IP "169.254.13.37"

#include "snesclassic.h"
#include "snesclassicfactory.h"

SNESClassicFactory::SNESClassicFactory()
{
    socket = new QTcpSocket();
    device = nullptr;
    checkAliveTimer.setInterval(1000);
    connect(&checkAliveTimer, &QTimer::timeout, this, &SNESClassicFactory::aliveCheck);
}

void SNESClassicFactory::executeCommand(QByteArray toExec)
{
    sDebug() << "Executing : " << toExec;
    writeSocket("CMD " + toExec + "\n");
}

void SNESClassicFactory::writeSocket(QByteArray toWrite)
{
    sDebug() << ">>" << toWrite;
    socket->write(toWrite);
}

QByteArray SNESClassicFactory::readCommandReturns(QTcpSocket* msocket)
{
    QByteArray toret;
    msocket->waitForReadyRead(50);
    forever {
        QByteArray data = msocket->readAll();
        sDebug() << "Reading" << data;
        if (data.isEmpty())
            break;
        toret += data;
        if (!msocket->waitForReadyRead(100))
            break;
    }
    toret.truncate(toret.size() - 4);
    return toret;
}

void SNESClassicFactory::findMemoryLocations()
{
    QByteArray pmap;
    executeCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    pmap = readCommandReturns(socket);
    QList<QByteArray> memEntries = pmap.split('\n');
    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        bool ok;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);
        sDebug() << ls.at(0);
        if (ls.at(1) == "5092")
            sramLocation = ls.at(0).toULong(&ok, 16) + 0x26E0;
        if (ls.at(1) == "6444")
            ramLocation = ls.at(0).toULong(&ok, 16) + 0x121BF4;
        if (ls.at(1) == "8196")
            romLocation = ls.at(0).toULong(&ok, 16) + 0x38;
    }
    sDebug() << "Locations : ram/sram/rom" << QString::number(ramLocation, 16) << QString::number(sramLocation, 16) << QString::number(romLocation, 16);
}

bool    SNESClassicFactory::checkStuff()
{
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    if (!data.isEmpty())
    {
        canoePid = data.trimmed();
        // canoe in demo mode is useless
        executeCommand("ps | grep canoe-shvc | grep -v grep");
        QByteArray canoeArgs = readCommandReturns(socket);
        if (canoeArgs.indexOf("-resume") != -1)
        {
            m_attachError = tr("SNES Classic emulator is running in demo mode.");
            return false;
        }
        findMemoryLocations();
        if (ramLocation != 0 && romLocation != 0 && sramLocation != 0)
        {
            return true;
        } else {
            m_attachError = tr("Can't find memory locations, try restarting the game.");
            return false;
        }
    } else {
        m_attachError = tr("The SNES Classic emulator is not running.");
    }
    return false;
}

void SNESClassicFactory::aliveCheck()
{
    QByteArray oldPid = canoePid;
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    data = data.trimmed();
    // Canoe not running anymore
    if (data.isEmpty())
    {
        sDebug() << "Closing the device, Canoe not running anymore";
        device->close();
        //checkAliveTimer.setInterval(1500);
        //canoePid = data.trimmed();
        checkAliveTimer.stop();
        return ;
    }
    canoePid = data.trimmed();
    if (canoePid == oldPid)
        return ;
    if (checkStuff())
    {
        sDebug() << "Updating device infos";
        device->canoePid = canoePid;
        device->setMemoryLocation(ramLocation, sramLocation, romLocation);
    } else {
        sDebug() << "Closing the device, can't find location";
        device->close();
    }
}

QStringList SNESClassicFactory::listDevices()
{
    QStringList toret;
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        sDebug() << "Trying to connect to serverstuff";
        socket->connectToHost(SNES_CLASSIC_IP, 1042);
        socket->waitForConnected(100);
    }
    sDebug() << socket->state();
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        if (checkStuff())
        {
            if (device == nullptr)
            {
                sDebug() << "Creating SNES Classic device";
                device = new SNESClassic();
                m_devices.append(device);
                checkAliveTimer.start();
            }
            if (device->state() == ADevice::CLOSED)
                device->sockConnect();
            device->canoePid = canoePid;
            device->setMemoryLocation(ramLocation, sramLocation, romLocation);
            //device->setState(ADevice::READY);
            return toret << "SNES Classic";
        } else {
            return toret;
        }

    } else {
        m_attachError = tr("Can't connect to the SNES Classic.");
    }
    sDebug() << "Not ready";
    return toret;
}

ADevice *SNESClassicFactory::attach(QString deviceName)
{
    m_attachError = "";
    if (deviceName != "SNES Classic")
        return nullptr;
    //listDevices();
    if (device != nullptr)
        sDebug() << "Attach" << device->state();
    if (device != nullptr && device->state() != ADevice::CLOSED)
        return device;
    return nullptr;
}

bool SNESClassicFactory::deleteDevice(ADevice *)
{
    return false;
}

QString SNESClassicFactory::status()
{
    listDevices();
    if (device == nullptr || device->state() == ADevice::CLOSED)
        return m_attachError;
    else
        return tr("SNES Classic ready.");
}

QString SNESClassicFactory::name() const
{
    return "SNES Classic (Hakchi2CE)";
}
