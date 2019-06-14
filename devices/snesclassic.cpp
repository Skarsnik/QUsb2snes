#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>

#include "../rommapping/rominfo.h"
#include "snesclassic.h"

Q_LOGGING_CATEGORY(log_snesclassic, "SNESClassic")
#define sDebug() qCDebug(log_snesclassic)
#define sInfo() qCInfo(log_snesclassic)


SNESClassic::SNESClassic()
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(3);
    alive_timer.setInterval(2000);
    socket = new QTcpSocket();
    connect(&alive_timer, &QTimer::timeout, this, &SNESClassic::onAliveTimeout);
    connect(&m_timer, &QTimer::timeout, this, &SNESClassic::onTimerOut);
    connect(socket, &QIODevice::readyRead, this, &SNESClassic::onSocketReadReady);
    connect(socket, &QTcpSocket::disconnected, this, &SNESClassic::onSocketDisconnected);
    connect(socket, &QTcpSocket::connected, [=] {
        sDebug() << "Connected to serverstuff";
        m_state = READY;
    });
    sDebug() << "Creating SNES Classic device";
    m_state = CLOSED;
    sramLocation = 0;
    romLocation = 0;
    ramLocation = 0;
    requestInfo = false;
    cmdWasGet = false;
}

//5757524954455F4D454D20373333203165343365312032390A201D7029FF
//5757524954455F4D454D20373333203165343365312032390A201D7029FF


void SNESClassic::onSocketReadReady()
{
    if (m_state == CLOSED)
        return;
    QByteArray data = socket->readAll();
    sDebug() << "Read stuff on socket " << cmdWasGet << " : " << data.size();
    sDebug() << "<<" << data << data.toHex();
    if (cmdWasGet)
    {
        getData += data;
        if (getData.left(9) == QByteArray::fromHex("00004552524f520000") || (getSize == 9 && getData.left(10) == QByteArray::fromHex("00004552524f52000000")))
        {
            sDebug() << "Error doing a get memory";
            socket->disconnectFromHost();
            return ;
        }
        if (getData.size() == static_cast<int>(getSize))
        {
            if (!requestInfo)
                emit getDataReceived(getData);
            else {
                dataRead = getData;
                requestInfo = false;
            }
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
            socket->disconnectFromHost();
    }
    return ;
cmdFinished:
    m_state = READY;
    emit commandFinished();
}


void SNESClassic::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    Q_UNUSED(space)
    m_state = BUSY;
    quint64 memAddr = 0;
    if (addr >= 0xF50000 && addr < 0xF70000)
    {
        memAddr = addr - 0xF50000 + ramLocation;
        //sDebug() << "RAM read";
    }
    if (addr >= 0xE00000 && addr < 0xF50000)
    {
        //sDebug() << "SRAM read";
        memAddr = addr - 0xE00000 + sramLocation;
    }
    if (addr < 0xE00000)
    {
        //sDebug() << "ROM read";
        memAddr = addr + romLocation;
    }
    sDebug() << "Get Addr" << memAddr;
    cmdWasGet = true;
    getSize = size;
    getData.clear();
    writeSocket("READ_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
}

void SNESClassic::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void SNESClassic::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    Q_UNUSED(space)
    sDebug() << "Put address" << addr;
    m_state = BUSY;
    quint64 memAddr = 0;
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
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void SNESClassic::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    Q_UNUSED(flags)
    putAddrCommand(space, addr, size);
}

void SNESClassic::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
    Q_UNUSED(opcode)
    Q_UNUSED(space)
    Q_UNUSED(flags)
    Q_UNUSED(arg)
    Q_UNUSED(arg2)
}

void SNESClassic::infoCommand()
{
    m_state = BUSY;
    sDebug() << "Requested Info";
    requestInfo = true;
    getAddrCommand(SD2Snes::space::SNES, 0xFFC0, 32);
}

void SNESClassic::setState(ADevice::State state)
{
    m_state = state;
}

void SNESClassic::writeData(QByteArray data)
{
    sDebug() << ">>" << data;
    socket->write(data);
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
    struct rom_infos* rInfos = get_rom_info(data.data());
    USB2SnesInfo info;
    info.romPlaying = rInfos->title;
    free(rInfos);
    info.version = "1.0.0";
    info.deviceName = "SNES Classic";
    info.flags << getFlagString(USB2SnesWS::NO_CONTROL_CMD);
    info.flags << getFlagString(USB2SnesWS::NO_FILE_CMD);
    return info;
}

QList<ADevice::FileInfos> SNESClassic::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI)
    return QList<ADevice::FileInfos>();
}

void SNESClassic::setMemoryLocation(unsigned int ramLoc, unsigned int sramLoc, unsigned int romLoc)
{
    ramLocation = ramLoc;
    sramLocation = sramLoc;
    romLocation = romLoc;
}

bool SNESClassic::open()
{
    return m_state == READY;
}

void SNESClassic::close()
{
    sDebug() << "Closing";
    m_state = CLOSED;
    socket->disconnectFromHost();
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
    /*QTcpSocket tmpSock;
    tmpSock.connectToHost(SNES_CLASSIC_IP, 1042);
    sDebug() << "Alive timer connection : " << tmpSock.waitForConnected(50);
    tmpSock.write("CMD pidof canoe-shvc\n");
    QByteArray tmp = readCommandReturns(tmpSock);
    if (tmp.trimmed() != canoePid)
        socket->close();
    tmpSock.disconnectFromHost();*/
}

void SNESClassic::onSocketDisconnected()
{
    sInfo() << "disconnected from serverstuff";
    m_state = CLOSED;
    emit closed();
}


void SNESClassic::writeSocket(QByteArray toWrite)
{
    sDebug() << ">>" << toWrite;
    socket->write(toWrite);
}


void SNESClassic::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::putFile(QByteArray name, unsigned int size)
{
    Q_UNUSED(name)
    Q_UNUSED(size)
}

//TODO need to check for canoe still running the right rom I guess?
bool SNESClassic::canAttach()
{
    return m_state == READY || m_state == BUSY;
}

void SNESClassic::sockConnect(QString ip)
{
    sDebug() << "Connecting to serverstuff " << ip;
    socket->connectToHost(ip, 1042);
}
