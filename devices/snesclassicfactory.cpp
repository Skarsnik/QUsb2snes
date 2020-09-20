#include <QLoggingCategory>
#include <QSettings>

Q_LOGGING_CATEGORY(log_snesclassicfact, "SNESClassic Factory")
#define sDebug() qCDebug(log_snesclassicfact)
#define sInfo() qCInfo(log_snesclassicfact)

extern QSettings* globalSettings;
extern bool    dontLogNext;

#define SNES_CLASSIC_IP "169.254.13.37"

#include "snesclassic.h"
#include "snesclassicfactory.h"

SNESClassicFactory::SNESClassicFactory()
{
    socket = new QTcpSocket();
    device = nullptr;
    checkAliveTimer.setInterval(1000);
    connect(&checkAliveTimer, &QTimer::timeout, this, &SNESClassicFactory::aliveCheck);
    if (globalSettings->contains("SNESClassicIP"))
        snesclassicIP = globalSettings->value("SNESClassicIP").toString();
    else
        snesclassicIP = SNES_CLASSIC_IP;
    sInfo() << "SNES Classic device will try to connect to " << snesclassicIP;
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
    QByteArray toret = readSocketReturns(msocket);
    toret.truncate(toret.size() - 4);
    return toret;
}

QByteArray SNESClassicFactory::readSocketReturns(QTcpSocket* msocket)
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
    return toret;
}

#include <QtEndian>

void SNESClassicFactory::findMemoryLocations()
{
    QByteArray pmap;
    executeCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    pmap = readCommandReturns(socket);
    QList<QByteArray> memEntries = pmap.split('\n');

    sramLocation = 0;
    romLocation = 0;
    ramLocation = 0;


    /*Find pointer at 1d6000 , offset 9f84

Find pointer at 1e0000 , offset 1730
*/

    bool ok;
    uint32_t pid = canoePid.toULong(&ok);
    QString s;
    s.sprintf("READ_MEM %u %zx %u\n", pid, 0x1dff84, 4);
    writeSocket(s.toUtf8());
    QByteArray memory = readSocketReturns(socket);
    ramLocation = qFromLittleEndian<uint32_t>(memory.data()) + 0x20BEC;


    //(*0x1dff84) + 0x20BEC

    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);

        if (ls.at(1) == "5092")
        {
            sramLocation = ls.at(0).toULong(&ok, 16) + 0x26E0;
        }
        else if (ls.at(1) == "8196")
        {
            romLocation = ls.at(0).toULong(&ok, 16) + 0x38;
        }
        else
        {
            /*
            // this is horrible
            QString s;
            uint32_t base = ls.at(0).toULong(&ok, 16);
            uint32_t pid = canoePid.toULong(&ok);
            uint32_t size = ls.at(1).toUInt() * 1024;
            s.sprintf("READ_MEM %u %zx %u\n", pid, base, size);
            writeSocket(s.toUtf8());
            QByteArray memory = readCommandReturns(socket);

            unsigned char magic[] = {0x70, 0x54, 0x11, 0x15};
            QByteArray data((char*)magic, 4);
            const int OFFSET = 0x02FB; // Offset from start to magic bytes
            const int RAM_OFFSET = 0xBF4; // Where in emulator RAM the start is

            int dataLocation = memory.indexOf(data);
            if (dataLocation != -1)
            {
                uint32_t addr = base + (dataLocation - OFFSET);
                if(addr % 0x1000 == RAM_OFFSET)
                {
                    ramLocation = addr;
                }
            }
*/

        }
    }
    sDebug() << "Locations : ram/sram/rom" << QString::number(ramLocation, 16) << QString::number(sramLocation, 16) << QString::number(romLocation, 16);
}

bool    SNESClassicFactory::checkStuff()
{
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    if (!data.isEmpty())
    {
        executeCommand("canoe-shvc --version");
        QByteArray version = readCommandReturns(socket);
        if(QString(version) != "fc349ac43140141277d2d6f964c1ee361fcd20ca\n")
        {
            m_attachError = tr("SNES Classic emulator is not v2.0.14 - has hash " + version);
            return 0;
        }


        auto oldCanoePid = canoePid;
        canoePid = data.trimmed();
        if(oldCanoePid != canoePid)
        {
            ramLocation = 0;
            romLocation = 0;
            sramLocation = 0;
        }
        // canoe in demo mode is useless
        executeCommand("ps | grep canoe-shvc | grep -v grep");
        QByteArray canoeArgs = readCommandReturns(socket);
        if (canoeArgs.indexOf("-resume") != -1)
        {
            m_attachError = tr("SNES Classic emulator is running in demo mode.");
            return false;
        }
        if (ramLocation == 0 || romLocation == 0 || sramLocation == 0)
        {
            findMemoryLocations();
        }
        if (ramLocation == 0 || romLocation == 0 || sramLocation == 0)
        {
            m_attachError = tr("Can't find memory locations, try restarting the game.");
            return false;
        }
        else
        {
            return true;
        }
    } else {
        m_attachError = tr("The SNES Classic emulator is not running.");
    }
    return false;
}

void SNESClassicFactory::aliveCheck()
{
    QByteArray oldPid = canoePid;
    dontLogNext = true;
    executeCommand("pidof canoe-shvc");
    QByteArray data = readCommandReturns(socket);
    dontLogNext = false;
    data = data.trimmed();
    // Canoe not running anymore
    if (data.isEmpty())
    {
        sDebug() << "Closing the device, Canoe not running anymore";
        device->close();
        //checkAliveTimer.setInterval(1500);
        //canoePid = data.trimmed();
        checkAliveTimer.stop();
        ramLocation = 0;
        romLocation = 0;
        sramLocation = 0;
        return ;
    }
    canoePid = data.trimmed();
    if (canoePid == oldPid)
    {
        return ;
    }
    ramLocation = 0;
    romLocation = 0;
    sramLocation = 0;
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
        socket->connectToHost(snesclassicIP, 1042);
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
            }
            if (device->state() == ADevice::CLOSED)
            {
                device->sockConnect(snesclassicIP);
                checkAliveTimer.start();
            }
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
