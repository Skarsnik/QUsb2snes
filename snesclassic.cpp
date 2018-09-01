#include "snesclassic.h"

#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>

Q_LOGGING_CATEGORY(log_snesclassic, "SNESClassic")
#define sDebug() qCDebug(log_snesclassic)

#define SNES_CLASSIC_IP "169.254.13.37"
//#define MEMSTUFF_PATH "/var/lib/hakchi/rootfs/memstuff"

SNESClassic::SNESClassic()
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(3);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(onSocketReadReady()));
    connect(&socket, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));
    sDebug() << "Creating SNES Classic device";
    m_state = CLOSED;
    sramLocation = 0;
    romLocation = 0;
    ramLocation = 0;
    cmdWasGet = false;
}

//5757524954455F4D454D20373333203165343365312032390A201D7029FF
//5757524954455F4D454D20373333203165343365312032390A201D7029FF


void SNESClassic::onSocketReadReady()
{
    if (m_state == CLOSED)
        return;
    QByteArray data = socket.readAll();
    sDebug() << "Read stuff on socket " << cmdWasGet << " : " << data.size();
    if (cmdWasGet)
    {
        getData += data;
        if (getData.size() == getSize)
        {
            sDebug() << getData;
            emit getDataReceived(getData);
            getData.clear();
            getSize = 0;
            cmdWasGet = false;
            goto cmdFinished;

        }
    } else { // Should be put command
        sDebug() << data;
        if (data == "OK\n")
            goto cmdFinished;
        if (data == "KO\n") // write command fail, let's close
            socket.close();
    }
    return ;
cmdFinished:
    emit commandFinished();
    m_state = READY;
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
    getSize = size;
    getData.clear();
    writeSocket("READ_MEM " + canoePid.toLatin1() + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
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
    cmdWasGet = false;
    sDebug() << "Put address" << QString::number(addr, 16) << QString::number(memAddr, 16);
    writeSocket("WRITE_MEM " + canoePid.toLatin1() + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
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
    sDebug() << ">>" << data;
    socket.write(data);
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
    return m_state == READY;
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


void SNESClassic::onSocketDisconnected()
{
    m_state = CLOSED;
    emit closed();
}

void SNESClassic::findMemoryLocations()
{
    QByteArray pmap;
    executeCommand(QByteArray("pmap ") + canoePid.toLatin1() + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon\n");
    pmap = readCommandReturns();
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

void SNESClassic::executeCommand(QByteArray toExec)
{
    sDebug() << "Executing : " << toExec;
    writeSocket("CMD " + toExec + "\n");
}

void SNESClassic::writeSocket(QByteArray toWrite)
{
    sDebug() << ">>" << toWrite;
    socket.write(toWrite);
}

QByteArray SNESClassic::readCommandReturns()
{
    QByteArray toret;
    socket.waitForReadyRead(50);
    forever {
        QByteArray data = socket.readAll();
        sDebug() << "Reading" << data;
        if (data.isEmpty())
            break;
        toret += data;
        if (!socket.waitForReadyRead(50))
            break;
    }
    toret.truncate(toret.size() - 4);
    return toret;
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

//TODO need to check for canoe still running the right rom I guess?
bool SNESClassic::canAttach()
{
    if (m_state == READY || m_state == BUSY)
        return true;
    if (socket.state() == QAbstractSocket::UnconnectedState)
    {
        sDebug() << "Trying to connect to serverstuff";
        socket.connectToHost(SNES_CLASSIC_IP, 1042);
        socket.waitForConnected(200);
    }
    sDebug() << socket.state();
    if (socket.state() == QAbstractSocket::ConnectedState)
    {
        executeCommand("pidof canoe-shvc");
        QByteArray data = readCommandReturns();
        if (!data.isEmpty())
        {
            canoePid = data.trimmed();
            // canoe in demo mode is useless
            executeCommand("ps | grep canoe-shvc | grep -v grep");
            QByteArray canoeArgs = readCommandReturns();
            if (canoeArgs.indexOf("-resume") != -1)
                return false;
            findMemoryLocations();
            if (ramLocation != 0 && romLocation != 0 && sramLocation != 0)
            {
                m_state = READY;
                return true;
            } else {
                return false;
            }
        }
    }
    sDebug() << "Not ready";
    return false;
}
