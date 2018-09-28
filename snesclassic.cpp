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
    alive_timer.setInterval(2000);
    connect(&alive_timer, SIGNAL(timeout()), this, SLOT(onAliveTimeout()));
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
    sDebug() << "<<" << data;
    if (cmdWasGet)
    {
        getData += data;
        if (getData.left(9) == "\0\0ERROR\0\0" || (getSize == 9 && getData.left(10) == "\0\0ERROR\0\0\0"))
        {
            socket.disconnectFromHost();
            return ;
        }
        if (getData.size() == getSize)
        {
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
            socket.disconnectFromHost();
    }
    return ;
cmdFinished:
    m_state = READY;
    emit commandFinished();
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
    writeSocket("READ_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
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
    writeSocket("WRITE_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
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

void    SNESClassic::onAliveTimeout()
{
    return ;
    QTcpSocket tmpSock;
    tmpSock.connectToHost(SNES_CLASSIC_IP, 1042);
    sDebug() << "Alive timer connection : " << tmpSock.waitForConnected(50);
    tmpSock.write("CMD pidof canoe-shvc\n");
    QByteArray tmp = readCommandReturns(tmpSock);
    if (tmp.trimmed() != canoePid)
        socket.close();
    tmpSock.disconnectFromHost();
}

void SNESClassic::onSocketDisconnected()
{
    m_state = CLOSED;
    alive_timer.stop();
    emit closed();
}

void SNESClassic::findMemoryLocations()
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

QByteArray SNESClassic::readCommandReturns(QTcpSocket& msocket)
{
    QByteArray toret;
    msocket.waitForReadyRead(50);
    forever {
        QByteArray data = msocket.readAll();
        sDebug() << "Reading" << data;
        if (data.isEmpty())
            break;
        toret += data;
        if (!msocket.waitForReadyRead(50))
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
        socket.waitForConnected(100);
    }
    sDebug() << socket.state();
    if (socket.state() == QAbstractSocket::ConnectedState)
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
                m_attachError = "SNES Classic emulator is running in demo mode";
                return false;
            }
            findMemoryLocations();
            if (ramLocation != 0 && romLocation != 0 && sramLocation != 0)
            {
                m_state = READY;
                alive_timer.start();
                return true;
            } else {
                m_attachError = "Can't find memory location, try restarting the emulator";
                return false;
            }
        } else {
            m_attachError = "The SNES Classic emulator is not running";
        }
    } else {
        m_attachError = "Can't connect to the SNES Classic";
    }
    sDebug() << "Not ready";
    return false;
}
