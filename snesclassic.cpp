#include "snesclassic.h"

#include <QEventLoop>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_snesclassic, "SNESClassic")
#define sDebug() qCDebug(log_snesclassic)

#define SNES_CLASSIC_IP "169.254.13.37"
#define MEMSTUFF_PATH "/var/lib/hakchi/rootfs/memstuff"

SNESClassic::SNESClassic()
{
    telCo = new TelnetConnection(SNES_CLASSIC_IP, 23, "root", "");
    m_timer.setSingleShot(true);
    m_timer.setInterval(3);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(telCo, SIGNAL(commandReturn(QByteArray)), this, SLOT(onTelnetCommandReturned(QByteArray)));
    connect(telCo, SIGNAL(disconnected()), this, SLOT(onTelnetDisconnected()));
    sDebug() << "Creating SNES Classic device";
    m_state = CLOSED;
}




void SNESClassic::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    m_state = BUSY;
    quint64 memAddr;
    if (addr >= 0xF50000 && addr < 0xF70000)
    {
        memAddr = addr - 0xF50000 + ramLocation;
        sDebug() << "RAM read";
    }
    if (addr >= 0xE00000 && addr < 0xF50000)
    {
        sDebug() << "SRAM read";
        memAddr = addr - 0xE00000 + sramLocation;
    }
    if (addr < 0xE00000)
    {
        sDebug() << "ROM read";
        memAddr = addr + romLocation;
    }
    sDebug() << "Get Addr" << memAddr;
    cmdWasGet = true;
    telCo->executeCommand(MEMSTUFF_PATH " " + canoePid + " read " + QString::number(memAddr, 16) + " " + QString::number(size));
}

void SNESClassic::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    sDebug() << "Put address" << addr;
    m_state = BUSY;
    quint64 memAddr;
    if (addr >= 0xF50000 && addr < 0xF70000)
        memAddr = addr - 0xF50000 + ramLocation;
    if (addr >= 0xE00000 && addr < 0xF50000)
        memAddr = addr - 0xE00000 + sramLocation;
    if (addr < 0xE00000)
        memAddr = addr + romLocation;
    sDebug() << "Put address" << QString::number(addr, 16) << QString::number(memAddr, 16);
    putAddr = memAddr;
    putSize = size;
}

void SNESClassic::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}

void SNESClassic::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
}

void SNESClassic::infoCommand()
{
    m_state = BUSY;
    sDebug() << "Requested Info";
    m_timer.start();
}

void SNESClassic::writeData(QByteArray data)
{
    static unsigned int receivedSize = 0;
    static QByteArray received = QByteArray();
    receivedSize += data.size();
    received += data;
    if (receivedSize == putSize)
    {
        telCo->executeCommand(MEMSTUFF_PATH "     " + canoePid + " write " + QString::number(putAddr, 16) + " " + QString::number(putSize) + " " + received.toHex());
        receivedSize = 0;
        received.clear();
    }
}

QString SNESClassic::name() const
{
    return "SNES Classic";
}

bool SNESClassic::hasFileCommands()
{
    return false;
}

bool SNESClassic::hasControlCommands()
{
    return false;
}

USB2SnesInfo SNESClassic::parseInfo(const QByteArray &data)
{
    USB2SnesInfo info;
    info.romPlaying = "No Info";
    info.version = "1.0.0";
    return info;
}

QList<ADevice::FileInfos> SNESClassic::parseLSCommand(QByteArray &dataI)
{
    return QList<ADevice::FileInfos>();
}

bool SNESClassic::open()
{
    sDebug() << "Trying to connect to telnet";
    QEventLoop loop;
    connect(telCo, SIGNAL(connected()), &loop, SLOT(quit()));
    connect(telCo, SIGNAL(error()), &loop, SLOT(quit()));
    telCo->conneect();
    loop.exec();
    sDebug() << telCo->state();
    if (telCo->state() == TelnetConnection::Connected)
    {
        QByteArray data = telCo->syncExecuteCommand("pidof canoe-shvc");
        if (!data.isEmpty())
        {
            canoePid = data.trimmed();
            findMemoryLocations();
            m_state = READY;
            return true;
        }
    }
    sDebug() << "Not ready";
    return false;
}

void SNESClassic::close()
{
}

void SNESClassic::onTimerOut()
{
    m_state = READY;
    sDebug() << "Fake command finished";
    emit commandFinished();
}

void SNESClassic::onTelnetCommandReturned(QByteArray data)
{
    if (m_state == BUSY)
    {
        if (cmdWasGet)
        {
            emit getDataReceived(QByteArray::fromHex(data.trimmed()));
            cmdWasGet = false;
        }
        m_state = READY;
        emit commandFinished();
    }
}

void SNESClassic::onTelnetDisconnected()
{
    m_state = CLOSED;
    emit closed();
}

void SNESClassic::findMemoryLocations()
{
    QByteArray pmap = telCo->syncExecuteCommand("pmap " + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
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

void SNESClassic::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
}

void SNESClassic::fileCommand(SD2Snes::opcode op, QByteArray args)
{
}

void SNESClassic::controlCommand(SD2Snes::opcode op, QByteArray args)
{
}

void SNESClassic::putFile(QByteArray name, unsigned int size)
{
}
