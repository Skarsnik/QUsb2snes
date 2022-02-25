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

#include "luabridgedevice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include "../rommapping/rommapping.h"
#include "../rommapping/rominfo.h"

Q_LOGGING_CATEGORY(log_luaBridgeDevice, "LUABridgeDevice")
#define sDebug() qCDebug(log_luaBridgeDevice)


#define setState(X_STATE) sDebug() << "Changed state to " << X_STATE << "in" <<  __func__; m_state = X_STATE;

LuaBridgeDevice::LuaBridgeDevice(QTcpSocket* sock, QString name)
{
    timer.setInterval(3);
    timer.setSingleShot(true);
    setState(CLOSED);
    m_socket = sock;
    m_name = name;
    infoReq = false;
    romMapping = LoROM;
    sDebug() << "LUA bridge device created";
    sock->write("Version\n");
    bizhawk = false;
    if (sock->waitForReadyRead(50))
    {
        QByteArray data = sock->readAll();
        sDebug() << "Version reply : " << data;
        if (data.indexOf("BizHawk") != -1)
            bizhawk = true;
    }
    getRomMapping();
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
}


// FIXME
void    LuaBridgeDevice::getRomMapping()
{
    sDebug() << "Trying to get ROM mapping";
    if (bizhawk)
        m_socket->write("Read|" + QByteArray::number(0x7FC0) + "|32|CARTROM\n");
    else
        m_socket->write("Read|" +  QByteArray::number(0x00FFC0) + "|32\n");
    if (m_socket->waitForReadyRead(100))
    {
        QByteArray data;
        QByteArray dataRead = m_socket->readAll();
        QJsonDocument   jdoc = QJsonDocument::fromJson(dataRead);
        QJsonObject     job = jdoc.object();
        QJsonArray      aData = job["data"].toArray();
        //sDebug() << aData;
        foreach (QVariant v, aData.toVariantList())
        {
            data.append(static_cast<char>(v.toInt()));
        }
        sDebug() << data.toHex();
        //sDebug() << data.at(21);
        struct rom_infos* rInfos = get_rom_info(data.data());
        gameName = rInfos->title;
        sDebug() << gameName << rInfos->title;
        romMapping = rInfos->type;
        free(rInfos);
    }
    sDebug() << "ROM is " << rommapping_to_name[romMapping] << romMapping;
}

/*
 *
 * "0": "WRAM"
"1": "CARTROM"
"2": "CARTRAM"
"3": "VRAM"
"4": "OAM"
"5": "CGRAM"
"6": "APURAM"
"7": "System Bus"
 * */

QPair<QByteArray, unsigned int>    LuaBridgeDevice::getBizHawkAddress(unsigned int addr)
{
    QPair<QByteArray, unsigned int> toret;
    if (addr >= 0xF50000 && addr < 0xF70000)
    {
        toret.first = "WRAM";
        toret.second = addr - 0xF50000;
        return toret;
    }
    if (addr >= 0xE00000)
    {
        toret.first = "CARTRAM";
        toret.second = addr - 0xE00000;
        return toret;
    }
    toret.first = "CARTROM";
    toret.second = addr;
    return toret;
}

/* This is from asar code from https://github.com/RPGHacker/asar/blob/master/src/asar/libsmw.h#L125
*/

static int asarpctosnes(unsigned int addr2, int mapper)
{
    int addr = static_cast<unsigned int>(addr2);
    if (mapper==LoROM)
    {
        if (addr>=0x400000) return -1;
        addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
        return addr|0x800000;
    }
    if (mapper==HiROM)
    {
        if (addr>=0x400000) return -1;
        return addr|0xC00000;
    }
    if (mapper == ExLoROM)
    {
        if (addr>=0x800000) return -1;
        if (addr&0x400000)
        {
            addr-=0x400000;
            addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
            return addr;
        }
        else
        {
            addr=((addr<<1)&0x7F0000)|(addr&0x7FFF)|0x8000;
            return addr|0x800000;
        }
    }
    if (mapper == ExHiROM)
    {
        if (addr>=0x800000) return -1;
        if (addr&0x400000) return addr;
        return addr|0xC00000;
    }
    return -1;
}

unsigned int LuaBridgeDevice::getSnes9xAddress(unsigned int addr)
{
    if (addr >= 0xF50000 && addr < 0xF70000)
        return addr - 0xF50000 + 0x7E0000;
    if (addr >= 0xE00000)
    {
        enum rom_type mapping = romMapping;
        if (mapping == ExHiROM)
            mapping = HiROM;
        return static_cast<unsigned int>(rommapping_sram_pc_to_snes(addr - 0xE00000, mapping, false));
    }
    return static_cast<unsigned int>(asarpctosnes(addr, romMapping));
}

void LuaBridgeDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    Q_UNUSED(space)
    QByteArray toWrite;
    if (bizhawk)
    {
        auto info = getBizHawkAddress(addr);
        toWrite = "Read|" + QByteArray::number(info.second) + "|" + QByteArray::number(size) + "|"
                + info.first + "\n";
    } else {
        toWrite = "Read|" + QByteArray::number(getSnes9xAddress(addr)) + "|" + QByteArray::number(size) + "\n";
    }

    sDebug() << ">>" << toWrite;
    sDebug() << "Writen" << m_socket->write(toWrite) << "Bytes";
    setState(BUSY);
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    Q_UNUSED(space)
    setState(BUSY);
    putAddr = addr;
    putSize = size;
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    Q_UNUSED(flags)
    putAddrCommand(space, addr, size);
}


void LuaBridgeDevice::infoCommand()
{
    setState(BUSY);
    timer.start();
}

void LuaBridgeDevice::writeData(QByteArray data)
{
    static unsigned int receivedSize = 0;
    static QByteArray received = QByteArray();
    receivedSize += static_cast<unsigned int>(data.size());
    received += data;
    if (receivedSize == putSize)
    {
        QByteArray toSend;
        if (bizhawk)
        {
            auto info = getBizHawkAddress(putAddr);
            toSend = "Write|" + QByteArray::number(info.second) + "|" + info.first;
        } else {
            toSend = "Write|" + QByteArray::number(getSnes9xAddress(putAddr));
        }
        for (int i = 0; i < received.size(); i++)
            toSend += "|" + QByteArray::number(static_cast<unsigned char>(received.at(i)));
        toSend += "\n";
        sDebug() << ">>" << toSend;
        m_socket->write(toSend);
        receivedSize = 0;
        received.clear();
        setState(READY);
        emit commandFinished();
    }
}

QString LuaBridgeDevice::name() const
{
    return "Emu - " + m_name;
}

QString LuaBridgeDevice::luaName() const
{
    return m_name;
}

bool LuaBridgeDevice::hasFileCommands()
{
    return false;
}

bool LuaBridgeDevice::hasControlCommands()
{
    return false;
}

USB2SnesInfo LuaBridgeDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data)
    USB2SnesInfo info;
    info.romPlaying = gameName;
    info.version = "1.1.0";
    if (bizhawk)
        info.deviceName = "BizHawk";
    else
        info.deviceName = "Snes9x";
    info.flags << getFlagString(USB2SnesWS::NO_CONTROL_CMD);
    info.flags << getFlagString(USB2SnesWS::NO_FILE_CMD);
    return info;
}

QList<ADevice::FileInfos> LuaBridgeDevice::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI)
    return QList<ADevice::FileInfos>();
}

QTcpSocket *LuaBridgeDevice::socket()
{
    return m_socket;
}

bool LuaBridgeDevice::open()
{
    setState(READY);
    return true;
}

void LuaBridgeDevice::close()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->close();
}


void LuaBridgeDevice::onServerError()
{
    m_state = CLOSED;
    emit closed();
}

void LuaBridgeDevice::onClientReadyRead()
{
    static      QByteArray  dataRead = QByteArray();

    QByteArray  data = m_socket->readAll();
    dataRead += data;

    sDebug() << "<<" << data;
    if (dataRead.right(1) == "\n")
    {
        QJsonParseError jsError;
        QJsonDocument   jdoc = QJsonDocument::fromJson(dataRead, &jsError);
        if (jdoc.isNull())
        {
            sDebug() << jsError.errorString();
            setState(READY);
            emit protocolError();
            dataRead.clear();
            return ;
        }

        QJsonObject     job = jdoc.object();
        if (!job.contains("data"))
        {
            sDebug() << "JSON from lua does not contain the data";
            setState(READY);
            emit protocolError();
            dataRead.clear();
            return ;
        }
        QJsonArray      aData = job["data"].toArray();
        //sDebug() << aData;
        data.clear();
        foreach (QVariant v, aData.toVariantList())
        {
            data.append(static_cast<char>(v.toInt()));
        }
        //sDebug() << data;
        setState(READY);
        emit getDataReceived(data);
        emit commandFinished();
        dataRead.clear();
    }
}

void LuaBridgeDevice::onClientDisconnected()
{
    sDebug() << "Client disconnected";
    m_socket->deleteLater();
    m_state = CLOSED;
    m_socket = nullptr;
    emit closed();
}

void LuaBridgeDevice::onTimerOut()
{
    setState(READY);
    sDebug() << "Fake command finished";
    emit commandFinished();
}



void LuaBridgeDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void LuaBridgeDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void LuaBridgeDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void LuaBridgeDevice::putFile(QByteArray name, unsigned int size)
{
    Q_UNUSED(name)
    Q_UNUSED(size)
}

void LuaBridgeDevice::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
    Q_UNUSED(opcode)
    Q_UNUSED(space)
    Q_UNUSED(flags)
    Q_UNUSED(arg)
    Q_UNUSED(arg2)
}



void LuaBridgeDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
}
