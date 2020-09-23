#include <QEventLoop>
#include <QLoggingCategory>
#include <QThread>

#include "snesclassic.h"

Q_LOGGING_CATEGORY(log_snesclassic, "SNESClassic")
#define sDebug() qCDebug(log_snesclassic)
#define sInfo() qCInfo(log_snesclassic)
#define sError sInfo
#include <sstream>
#include <thread>

SNESClassic::SNESClassic()
{
    alive_timer.setInterval(200);
    socket = new QTcpSocket();
    connect(&alive_timer, &QTimer::timeout, this, &SNESClassic::onAliveTimeout);

    connect(socket, &QIODevice::readyRead, this, &SNESClassic::onSocketReadReady);
    connect(socket, &QTcpSocket::disconnected, this, &SNESClassic::onSocketDisconnected);
    connect(socket, &QTcpSocket::connected, this, &SNESClassic::onSocketConnected);
    sDebug() << "Creating SNES Classic device";
    m_state = CLOSED;
    sramLocation = 0;
    romLocation = 0;
    ramLocation = 0;
    c_rom_infos = nullptr;
}

void SNESClassic::socketCommandFinished()
{
    sDebug() << "socketfin";
    m_state = READY;
    alive_timer.stop();
    setCommand(SD2Snes::opcode::OPCODE_NONE);

    emit commandFinished();
}

void SNESClassic::socketCommandError()
{
    sDebug() << "socketerr";
    socket->disconnectFromHost();
    alive_timer.stop();
    setCommand(SD2Snes::opcode::OPCODE_NONE);
}

void SNESClassic::onSocketReadReady()
{
    if (m_state == CLOSED)
    {
        return;
    }
    const QByteArray data = socket->readAll();
    sDebug() << "Read stuff on socket " << lastCommandType << " : " << data.size();
    sDebug() << "<<" << data << data.toHex();

    CommandState commandState = CommandState::ERROR;

    switch(lastCommandType)
    {
        case SD2Snes::opcode::INFO:
        {
            commandState = handleRequestInfoResponse(data);
            break;
        }
        case SD2Snes::opcode::GET:
        {
            commandState = handleGetDataResponse(data);
            break;
        }
        case SD2Snes::opcode::PUT:
        {
            commandState = handlePutDataResponse(data);
            break;
        }
        case SD2Snes::opcode::OPCODE_NONE:
        {
            assert(data.size() == 0);
            return;
        }
        default:
        {
            assert(false);
        }
    }

    switch(commandState)
    {
        case CommandState::ERROR:
        {
            qDebug() << "ERROR";
            socketCommandError();
            break;
        }
        case CommandState::IN_PROGRESS:
        {
            qDebug() << "IN_PROG";
            break;
        }
        case CommandState::FINISHED:
        {
            qDebug() << "FIN!";
            socketCommandFinished();
            break;
        }
    }
}

QByteArray SNESClassic::getInfoCommand(rom_type romType)
{
    infoRequestType = romType;

    switch(romType)
    {
    case LoROM:
        return "READ_MEM " + canoePid + " " + QByteArray::number(0x7FC0 + romLocation, 16) + " " + QByteArray::number(32) + "\n";
    case HiROM:
        return "READ_MEM " + canoePid + " " + QByteArray::number(0xFFC0 + romLocation, 16) + " " + QByteArray::number(32) + "\n";
    default:
        assert(false);
    }
    Q_UNREACHABLE();
}

CommandState SNESClassic::handleRequestInfoResponse(const QByteArray& data)
{
    if (c_rom_infos != nullptr)
    {
        free(c_rom_infos);
    }
    c_rom_infos = get_rom_info(data.data());
    if (rom_info_make_sense(c_rom_infos, infoRequestType))
    {
        return CommandState::FINISHED;
    }
    else if(infoRequestType == LoROM)
    {
        // Checking HiROM
        writeSocket(getInfoCommand(HiROM));
        return CommandState::IN_PROGRESS;
    }
    else
    {
        sError() << "Couldn't get rom info";
        return CommandState::ERROR;
    }
}

CommandState SNESClassic::handleGetDataResponse(const QByteArray& data)
{
    getData += data;
    if (getData.left(9) == QByteArray::fromHex("00004552524f520000") || (getSize == 9 && getData.left(10) == QByteArray::fromHex("00004552524f52000000")))
    {
        sError() << "Error doing a get memory";
        socket->disconnectFromHost();
        return CommandState::ERROR;
    }
    else if (getData.size() == static_cast<int>(getSize))
    {
        emit getDataReceived(getData);
        getData.clear();
        getSize = 0;
        return CommandState::FINISHED;
    }
    // Else, haven't filled buffer for get request
    // Can this even happen?
    return CommandState::ERROR;
}

CommandState SNESClassic::handlePutDataResponse(const QByteArray& data)
{
    sDebug() << data;
    if (data == "OK\n")
    {
        return CommandState::FINISHED;
    }
    else if (data == "KO\n") // write command fail, let's close
    {
        sError() << "Put command failed!";
        return CommandState::ERROR;
    }
    else
    {
        sError() << "Unknown put response " << data;
        return CommandState::ERROR;
    }
}

void SNESClassic::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
setCommand(SD2Snes::opcode::GET);
    Q_UNUSED(space)
    m_state = BUSY;
    quint64 memAddr = 0;
    if (addr >= 0xF50000 && addr < 0xF70000)
    {
        memAddr = addr - 0xF50000 + ramLocation;
    }
    if (addr >= 0xE00000 && addr < 0xF50000)
    {
        memAddr = addr - 0xE00000 + sramLocation;
    }
    if (addr < 0xE00000)
    {
        memAddr = addr + romLocation;
    }
    sDebug() << "Get Addr" << memAddr;

    getSize = size;
    getData.clear();
    writeSocket("READ_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
    alive_timer.start();
}

void SNESClassic::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
    assert(false);
}

void SNESClassic::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    setCommand(SD2Snes::opcode::PUT);
    Q_UNUSED(space)
    sDebug() << "Put address" << addr;
    m_state = BUSY;
    quint64 memAddr = 0;
    if (addr >= 0xF50000 && addr < 0xF70000)
    {
        memAddr = addr - 0xF50000 + ramLocation;
    }
    if (addr >= 0xE00000 && addr < 0xF50000)
    {
        memAddr = addr - 0xE00000 + sramLocation;
    }
    if (addr < 0xE00000)
    {
        memAddr = addr + romLocation;
    }

    lastPutWrite.clear();
    sDebug() << "Put address" << QString::number(addr, 16) << QString::number(memAddr, 16);
    writeSocket("WRITE_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
    alive_timer.start();
}

void SNESClassic::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    assert(false);
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void SNESClassic::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    Q_UNUSED(flags)
    putAddrCommand(space, addr, size);
}

void SNESClassic::infoCommand()
{
    setCommand(SD2Snes::opcode::INFO);
    m_state = BUSY;
    sDebug() << "Requested Info";
    writeSocket(getInfoCommand(LoROM));
}

void SNESClassic::writeData(QByteArray data)
{
    lastPutWrite += data;
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
    // TODO: set flags from has*
    // TODO: hoist into ADevice
    // TODO: let devices declare functionality with opcodes???
    Q_UNUSED(data)
    USB2SnesInfo info;
    info.romPlaying = c_rom_infos->title;
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

void SNESClassic::onAliveTimeout()
{
    alive_timer.stop();
    sDebug() << "An operation timeout, ServerStuff has an issue";
    disconnect(socket, &QTcpSocket::disconnected, this, &SNESClassic::onSocketDisconnected);
    socket->disconnectFromHost();
    socket->waitForDisconnected(200);
    sDebug() << "Reconnecing to SNES classic";
    // Connect put the device in READY state
    // We want to keep it BUSY
    disconnect(socket, &QTcpSocket::connected, this, &SNESClassic::onSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &SNESClassic::onSocketDisconnected);
    socket->connectToHost(myIp, 1042);

    // TODO: introduce command index to serverstuff so that it can discard already used commands?
    // TODO: this can fail
    if (socket->waitForConnected(100))
    {
        if(lastCommandType != SD2Snes::opcode::OPCODE_NONE)
        {
            // don't bypass in this case, we need to reset the last written timer
            writeSocket(lastCmdWrite);
            if (lastCommandType == SD2Snes::opcode::PUT)
            {
                // bypass writeSocket and writeData, we don't want to mess with that state
                socket->write(lastPutWrite);
            }
        }
    }
    connect(socket, &QTcpSocket::connected, this, &SNESClassic::onSocketConnected);
}

void SNESClassic::onSocketDisconnected()
{
    alive_timer.stop();
    sInfo() << "disconnected from serverstuff";
    m_state = CLOSED;
    emit closed();
}


void SNESClassic::writeSocket(QByteArray toWrite)
{    
    sDebug() << "writeSocket";
    assert(lastCommandType != SD2Snes::opcode::OPCODE_NONE);
    lastCmdWrite = toWrite;
    sDebug() << ">>" << toWrite;
    socket->write(toWrite);
}


void SNESClassic::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    assert(false);
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    assert(false);
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    assert(false);
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void SNESClassic::putFile(QByteArray name, unsigned int size)
{
    assert(false);
    Q_UNUSED(name)
    Q_UNUSED(size)
}

void SNESClassic::onSocketConnected()
{
    sDebug() << "Connected to serverstuff";
    m_state = READY;
}

void SNESClassic::sockConnect(QString ip)
{
    myIp = ip;
    sDebug() << "Connecting to serverstuff " << ip;
    socket->connectToHost(ip, 1042);
}

void SNESClassic::setCommand(SD2Snes::opcode command)
{
    if(command != SD2Snes::opcode::OPCODE_NONE)
    {
        if(lastCommandType != SD2Snes::opcode::OPCODE_NONE)
        {
            qDebug() << "About to assert because last command is " << lastCommandType << " and this command is " << command;
        }
        assert(lastCommandType == SD2Snes::opcode::OPCODE_NONE);
    }
    qDebug() << "set state to" << command;
    lastCommandType = command;
}
