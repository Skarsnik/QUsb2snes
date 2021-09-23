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



#include <QLoggingCategory>

#include "retroarchhost.h"

Q_LOGGING_CATEGORY(log_retroarchhost, "RetroArcHost")
#define sDebug() qCDebug(log_retroarchhost) << m_name


RetroArchHost::RetroArchHost(QString name, QObject *parent) : QObject(parent)
{
    m_name = name;
    m_port = 55355;
    readRamHasRomAccess = false;
    readMemoryAPI = false;
    commandTimeoutTimer.setInterval(500);
    commandTimeoutTimer.setSingleShot(true);
    connect(&socket, &QUdpSocket::readyRead, this, &RetroArchHost::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    connect(&socket, &QUdpSocket::errorOccurred, this, &RetroArchHost::errorOccured);
    connect(&socket, &QUdpSocket::errorOccurred, this, [this]{
        sDebug() << socket.error() << socket.errorString();
    });
#else
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &RetroArchHost::errorOccured);
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [this]{
        sDebug() << socket.error() << socket.errorString();
    });
#endif
    connect(&commandTimeoutTimer, &QTimer::timeout, this, &RetroArchHost::onCommandTimerTimeout);
    state = None;
}

void RetroArchHost::setHostAddress(QHostAddress addr, quint16 port)
{
    m_address = addr;
    m_port = port;
}

qint64 RetroArchHost::getMemory(unsigned int address, unsigned int size)
{
    int raAddress = translateAddress(address);
    if (raAddress == -1)
        return -1;
    QByteArray data = (readMemoryAPI ? "READ_CORE_MEMORY " : "READ_CORE_RAM ") + QByteArray::number(raAddress, 16) + " " + QByteArray::number(size) + "\n";
    getMemorySize = (int)size;
    doCommmand(data);
    state = GetMemory;
    return ++id;
}

qint64 RetroArchHost::writeMemory(unsigned int address, unsigned int size)
{
    state = WriteMemory;
    int raAddress = translateAddress(address);
    if (raAddress == -1)
        return -1;
    writeAddress = (int)raAddress;
    writeSize = (int)size;
    writeMemoryBuffer.clear();
    writeMemoryBuffer.reserve(writeSize);
    return ++id;
}

void RetroArchHost::writeMemoryData(QByteArray data)
{
    writeMemoryBuffer.append(data);
    if (writeSize == writeMemoryBuffer.size())
    {
        QByteArray data = (readMemoryAPI ? "WRITE_CORE_MEMORY " : "WRITE_CORE_RAM ") + QByteArray::number(writeAddress, 16) + " ";
        data.append(writeMemoryBuffer.toHex(' '));
        data.append('\n');
        doCommmand(data);
        commandTimeoutTimer.stop();
        if (!readMemoryAPI) { // old API does not send a reply
            sDebug() << "Write memory done";
            state = None;
            emit writeMemoryDone(id);
        }

    }
}

qint64 RetroArchHost::getInfos()
{
    sDebug() << m_name << "Doing info";
    doCommmand("VERSION");
    state = ReqInfoVersion;
    return ++id;
}

QVersionNumber RetroArchHost::version() const
{
    return m_version;
}

QByteArray RetroArchHost::getMemoryData() const
{
    return getMemoryDatas;
}

QString RetroArchHost::name() const
{
    return  m_name;
}

QString RetroArchHost::gameTitle() const
{
    return m_gameTile;
}

bool RetroArchHost::hasRomAccess() const
{
    if (readMemoryAPI)
        return false;
    return readRamHasRomAccess;
}

QHostAddress RetroArchHost::address() const
{
    return m_address;
}

QString RetroArchHost::lastInfoError() const
{
    return m_lastInfoError;
}

void RetroArchHost::setInfoFromRomHeader(QByteArray data)
{
    struct rom_infos* rInfos = get_rom_info(data);
    sDebug() << "From header : " << rInfos->title << rInfos->type;
    if (!readMemoryAPI)
        m_gameTile = rInfos->title;
    romType = rInfos->type;
    free(rInfos);
}


void RetroArchHost::makeInfoFail(QString error)
{
    state = None;
    sDebug() << "Info error " << error;
    m_lastInfoError = error;
    emit infoFailed(id);
}

void RetroArchHost::onReadyRead()
{
    while (socket.hasPendingDatagrams()) {
        QHostAddress sender;
        quint16 senderPort;
        QByteArray data;
        data.resize(socket.pendingDatagramSize());
        socket.readDatagram(data.data(), data.size(), &sender, &senderPort);
        sDebug() << "<<" << data << "from" << sender << senderPort;
        if (senderPort != m_port || !sender.isEqual(m_address, QHostAddress::TolerantConversion)) {
            sDebug() << "bad sender";
            continue;
        }
        onPacket(data);
    }
}

void RetroArchHost::onPacket(QByteArray& data)
{
    commandTimeoutTimer.stop();
    switch(state)
    {
        case GetMemory:
        {
            QList<QByteArray> tList = data.trimmed().split(' ');
            tList = tList.mid(2);
            if (tList.at(0) == "-1")
            {
                emit getMemoryFailed(id);
                break;
            }
            getMemoryDatas = QByteArray::fromHex(tList.join());
            emit getMemoryDone(id);
            break;
        }
        case WriteMemory:
        {
            if (readMemoryAPI)
            {
                sDebug() << "Write memory done";
                state = None;
                emit writeMemoryDone(id);
            }
            break;
        }

        /*
         * Info Stuff
        */
        case ReqInfoVersion:
        {
            QString raVersion = data.trimmed();
            m_version = QVersionNumber::fromString(raVersion);
            if (raVersion == "")
            {
                makeInfoFail("RetroArch did not reply to version");
                break;
            }
            sDebug() << "Version : " << m_version;
            readMemoryAPI = m_version >= QVersionNumber(1,9,0);
            if (readMemoryAPI)
            {
                doCommmand("GET_STATUS");
                state = ReqInfoStatus;
            } else {
                doCommmand("READ_CORE_RAM 0 1");
                state = ReqInfoRRAMZero;
            }
            break;
        }
        // These are the step for the newest RA with the READ_MEMORY api
        case ReqInfoStatus:
        {
            // GET_STATUS PAUSED super_nes,Secret of Evermore,crc32=5756f698
            QRegExp statusExp("^GET_STATUS (\\w+)(\\s.+)?");
            if (statusExp.indexIn(data))
            {
                makeInfoFail("RetroArch did not return a proper formated status reply");
                break;
            }
            QString status = statusExp.cap(1);
            QString infos = statusExp.cap(2).trimmed();
            QString plateform;
            QString game;

            sDebug() << status << infos;
            if (infos.isEmpty() == false)
            {
                plateform = infos.split(',').at(0);
                game = infos.split(',').at(1);
            }
            if (status == "CONTENTLESS" || plateform != "super_nes")
            {
                makeInfoFail("RetroArch not running a super nintendo game");
                break;
            }
            m_gameTile = game;
            state = ReqInfoRMemoryWRAM;
            doCommmand("READ_CORE_MEMORY 7E0000 1");
            break;
        }
        case ReqInfoRMemoryWRAM:
        {
            if (data.split(' ').at(2) == "-1")
            {
                makeInfoFail("Could not read WRAM");
                break;
            }
            state = ReqInfoRMemoryHiRomData;
            doCommmand("READ_CORE_MEMORY 40FFC0 32");

            break;
        }
        case ReqInfoRMemoryHiRomData:
        case ReqInfoRMemoryLoRomData:
        {
            QList<QByteArray> tList = data.trimmed().split(' ');
            tList.removeFirst();
            tList.removeFirst();
            if (tList.at(0) == "-1")
            {
                if (state == ReqInfoRMemoryHiRomData)
                {
                    doCommmand("READ_CORE_MEMORY FFC0 32");
                    state = ReqInfoRMemoryLoRomData;
                    break;
                } else {
                    // This should actually still be valid, since you can read wram
                    // but sram will be 'impossible' without the rom mapping
                    makeInfoFail("Can't get ROM info");
                    break;
                }
            }
            setInfoFromRomHeader(QByteArray::fromHex(tList.join()));
            emit infoDone(id);
            break;
        }
        // This is the part for the old RA Version
        case ReqInfoRRAMZero:
        {
            if (data == "READ_CORE_RAM 0 -1\n")
            {
                makeInfoFail("No game is running or core does not support memory read.");
                break;
            }
            doCommmand("READ_CORE_RAM 40FFC0 32");
            state = ReqInfoRRAMHiRomData;
            break;
        }
        case ReqInfoRRAMHiRomData:
        {
            QList<QByteArray> tList = data.trimmed().split(' ');
            if (tList.at(2) == "-1")
            {
                readRamHasRomAccess = false;
            } else {
                tList.removeFirst();
                tList.removeFirst();
                setInfoFromRomHeader(QByteArray::fromHex(tList.join()));
                readRamHasRomAccess = true;
            }
            emit infoDone(id);
            break;
        }
        default:
        {
            sDebug() << "Onreadyread with unknow state" << state;
            break;
        }
    }
}


void RetroArchHost::onCommandTimerTimeout()
{
    sDebug() << "Command timer timeout";
    if (state >= ReqInfoVersion && state <= ReqInfoMemory2)
    {
        emit makeInfoFail("Timeout on one of the command");
    } else {
        emit commandTimeout(id);
    }
    state = None;
}

// ref is https://github.com/tewtal/pusb2snes/blob/master/src/main/python/devices/retroarch.py


int RetroArchHost::translateAddress(unsigned int address)
{
    int addr = static_cast<int>(address);
    if (readMemoryAPI)
    {
        // ROM ACCESS is like whatever it seems, let's not bother with it
        if (addr < 0xE00000)
            return -1;// return rommapping_pc_to_snes(address, romType, false);
        if (addr >= 0xF50000 && addr <= 0xF70000)
            return 0x7E0000 + addr - 0xF50000;
        if (addr >= 0xE00000 && addr < 0xF70000)
            return rommapping_sram_pc_to_snes(address - 0xE00000, romType, false);
        return -1;
    } else {
        // Rom access
        if (addr < 0xE00000)
        {
            if (!hasRomAccess())
                return -1;
            if (romType == LoROM)
                return 0x800000 + (addr + (0x8000 * ((addr + 0x8000) / 0x8000)));
            if (romType == HiROM)
            {
                if (addr < 0x400000)
                    return addr + 0xC00000;
                else //exhirom
                    return addr + 0x400000;
            }
        }
        // WRAM access is pretty simple
        if (addr >= 0xF50000 && addr <= 0xF70000)
        {
            if (hasRomAccess()) {
                return addr - 0xF50000 + 0x7E0000;
            } else {
                return addr - 0xF50000;
            }
        }
        if (addr >= 0xE00000 && addr < 0xF70000)
        {
            if (!hasRomAccess())
                return addr - 0xE00000 + 0x20000;
            if (romType == LoROM)
                return addr - 0xE00000 + 0x700000;
            return lorom_sram_pc_to_snes(address - 0xE00000); // Test this
        }
        return -1;
    }
}

void RetroArchHost::doCommmand(QByteArray cmd)
{
    commandTimeoutTimer.start();
    writeSocket(cmd);
}

void RetroArchHost::writeSocket(QByteArray data)
{
    sDebug() << ">>" << data;
    auto written = socket.writeDatagram(data, m_address, m_port);
    if (written != data.size())
        sDebug() << "only written" << written << "/" << data.size() << "bytes";
}
