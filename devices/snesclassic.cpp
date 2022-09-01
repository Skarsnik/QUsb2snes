/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    alive_timer.setInterval(200);
    socket = new QTcpSocket(this);
    connect(&alive_timer, &QTimer::timeout, this, &SNESClassic::onAliveTimeout);
    connect(&m_timer, &QTimer::timeout, this, &SNESClassic::onTimerOut);
    connect(socket, &QIODevice::readyRead, this, &SNESClassic::onSocketReadReady);
    connect(socket, &QTcpSocket::disconnected, this, &SNESClassic::onSocketDisconnected);
    connect(socket, &QTcpSocket::connected, this, &SNESClassic::onSocketConnected);
    sDebug() << "Creating SNES Classic device";
    m_state = CLOSED;
    sramLocation = 0;
    romLocation = 0;
    ramLocation = 0;
    requestInfo = false;
    cmdWasGet = false;
    c_rom_infos = nullptr;
}

void SNESClassic::onSocketReadReady()
{
    static rom_type infoRequestType = LoROM;
    if (m_state == CLOSED)
        return;
    QByteArray data = socket->readAll();
    //sDebug() << "Read stuff on socket " << cmdWasGet << " : " << data.size();
    sDebug() << "<<" << data << data.toHex();
    if (requestInfo)
    {
        if (c_rom_infos != nullptr)
            free(c_rom_infos);
        c_rom_infos = get_rom_info(data.data());
        if (rom_info_make_sense(c_rom_infos, infoRequestType) || infoRequestType == HiROM)
        {
            infoRequestType = LoROM;
            requestInfo = false;
            goto cmdFinished;
        }

        // Checking HiROM
        writeSocket("READ_MEM " + canoePid + " " + QByteArray::number(0xFFC0 + romLocation, 16) + " " + QByteArray::number(32) + "\n");
        infoRequestType = HiROM;
        return;
    }
    if (cmdWasGet)
    {
        getData += data;
        if (getData.left(9) == QByteArray::fromHex("00004552524f520000") || (getSize == 9 && getData.left(10) == QByteArray::fromHex("00004552524f52000000")))
        {
            sDebug() << "Error doing a get memory";
            socket->disconnectFromHost();
            alive_timer.stop();
            return ;
        }
        if (getData.size() == static_cast<int>(getSize))
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
        {
            goto cmdFinished;
        }
        else if (data == "KO\n") // write command fail, let's close
        {
            socket->disconnectFromHost();
            alive_timer.stop();
        }
    }
    return ;
cmdFinished:
    m_state = READY;
    alive_timer.stop();
    emit commandFinished();
    return;
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
    alive_timer.start();
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
    cmdWasGet = false;
    lastPutWrite.clear();
    sDebug() << "Put address" << QString::number(addr, 16) << QString::number(memAddr, 16);
    writeSocket("WRITE_MEM " + canoePid + " " + QByteArray::number(memAddr, 16) + " " + QByteArray::number(size) + "\n");
    alive_timer.start();
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

void SNESClassic::infoCommand()
{
    m_state = BUSY;
    sDebug() << "Requested Info";
    requestInfo = true;
    writeSocket("READ_MEM " + canoePid + " " + QByteArray::number(0x7FC0 + romLocation, 16) + " " + QByteArray::number(32) + "\n");
}

void SNESClassic::setState(ADevice::State state)
{
    m_state = state;
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

void SNESClassic::onTimerOut()
{
    m_state = READY;
    sDebug() << "Fake command finished";
    emit commandFinished();
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
    if (socket->waitForConnected(100))
    {
        writeSocket(lastCmdWrite);
        if (!cmdWasGet)
        {
            writeSocket(lastPutWrite);
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
    lastCmdWrite = toWrite;
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
