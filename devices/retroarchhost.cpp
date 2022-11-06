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
#include <QRegularExpression>

#include "retroarchhost.h"

Q_LOGGING_CATEGORY(log_retroarchhost, "RetroArcHost")
#define sDebug() qCDebug(log_retroarchhost) << m_name


RetroArchHost::RetroArchHost(QString name, QObject *parent) : QObject(parent)
{
    m_name = name;
    m_port = 55355;
    readRamHasRomAccess = false;
    readMemoryAPI = false;
    useReadMemoryAPI = false;
    readMemoryHasRomAccess = false;
    lastId = -1;
    reqId = -1;
    writeId = -1;
    writeSize = 0;
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
        return -1; // error
    QByteArray data = (useReadMemoryAPI ? "READ_CORE_MEMORY " : "READ_CORE_RAM ") + QByteArray::number(raAddress, 16) + " " + QByteArray::number(size) + "\n";
    getMemorySize = (int)size;
    return queueCommand(data, GetMemory);
}

qint64 RetroArchHost::writeMemory(unsigned int address, unsigned int size)
{
    int raAddress = translateAddress(address);
    if (raAddress == -1)
        return -1; // error
    writeAddress = (int)raAddress;
    writeSize = (int)size;
    writeMemoryBuffer.clear();
    writeMemoryBuffer.reserve(writeSize);
    writeId = nextId();
    return writeId;
}

void RetroArchHost::writeMemoryData(qint64 id, QByteArray data)
{
    if (id != writeId) return;
    writeMemoryBuffer.append(data);
    assert(writeMemoryBuffer.size() <= writeSize);
    if (writeSize == writeMemoryBuffer.size())
    {
        QByteArray data = (useReadMemoryAPI ? "WRITE_CORE_MEMORY " : "WRITE_CORE_RAM ") + QByteArray::number(writeAddress, 16) + " ";
        data.append(writeMemoryBuffer.toHex(' '));
        data.append('\n');
        if (!useReadMemoryAPI) { // old API does not send a reply
            queueCommand(data, None, [this](qint64 sentId) {
                sDebug() << "Write memory done";
                emit writeMemoryDone(sentId);
            }, writeId);
        } else {
            queueCommand(data, WriteMemory, nullptr, writeId);
        }
    }
}

qint64 RetroArchHost::getInfos()
{
    if (state >= ReqInfoVersion && state <= ReqInfoMemory2) {
        sDebug() << m_name << "Waiting for info done";
        return reqId;
    } else {
        for (const auto& cmd: qAsConst(commandQueue)) {
            if (cmd.state == ReqInfoVersion) {
                sDebug() << m_name << "Waiting for info";
                return cmd.id;
            }
        }
        sDebug() << m_name << "Doing info";
        return queueCommand("VERSION", ReqInfoVersion);
    }
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
        return readMemoryHasRomAccess;
    return readRamHasRomAccess;
}

bool RetroArchHost::hasRomWriteAccess() const
{
    if (readMemoryAPI) return false; // at least with bsnes-mercury
    return hasRomAccess(); // old API untested
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
    emit infoFailed(reqId);
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
    // GET_STATUS PAUSED super_nes,Secret of Evermore,crc32=5756f698
    static const QRegularExpression statusExp("^GET_STATUS (\\w+)(\\s.+)?");
    commandTimeoutTimer.stop();

    switch(state)
    {
        /*
         * Memory Stuff
         */
        case GetMemory:
        {
            state = None;
            QList<QByteArray> tList = data.trimmed().split(' ');
            tList = tList.mid(2);
            if (tList.at(0) == "-1")
            {
                emit getMemoryFailed(reqId);
                break;
            }
            getMemoryDatas = QByteArray::fromHex(tList.join());
            emit getMemoryDone(reqId);
            break;
        }
        case WriteMemory:
        {
            state = None;
            if (readMemoryAPI)
            {
                sDebug() << "Write memory done";
                emit writeMemoryDone(reqId);
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
                doCommandNow({reqId, "GET_STATUS", ReqInfoStatus, nullptr});
            } else {
                doCommandNow({reqId, "READ_CORE_RAM 0 1", ReqInfoRRAMZero, nullptr});
            }
            break;
        }
        // These are the step for the newest RA with the READ_MEMORY api
        case ReqInfoStatus:
        {
            QRegularExpressionMatch match = statusExp.match(data);
            if (!match.hasMatch())
            {
                makeInfoFail("RetroArch did not return a proper formated status reply");
                break;
            }
            QString status = match.captured(1);
            QString infos = match.captured(2).trimmed();
            QString plateform;
            QString game;

            sDebug() << status << infos;
            if (infos.isEmpty() == false)
            {
                plateform = infos.split(',').at(0);
                game = infos.split(',').at(1);
            }
            // Because obviously, this is not a consistent string, so you get the core sometime...
            if (status == "CONTENTLESS" || (plateform != "super_nes" && !plateform.contains("snes")))
            {
                makeInfoFail("RetroArch not running a super nintendo game");
                break;
            }
            m_gameTile = game;
            doCommandNow({reqId, "READ_CORE_MEMORY 7E0000 1", ReqInfoRMemoryWRAM, nullptr});
            break;
        }
        case ReqInfoRMemoryWRAM:
        {
            if (data.split(' ').at(2) == "-1")
            {
                // fall back to READ_CORE_RAM (WRAM-only) for Snes9x core, RA>=1.9.2
                if (m_version >= QVersionNumber(1,9,2)) {
                    readMemoryAPI = false;
                    doCommandNow({reqId, "READ_CORE_RAM 0 1", ReqInfoRRAMZero, nullptr});
                } else {
                    makeInfoFail("Incompatible, please update RetroArch");
                }
                break;
            }
            doCommandNow({reqId, "READ_CORE_MEMORY 40FFC0 32", ReqInfoRMemoryHiRomData, nullptr});
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
                    doCommandNow({reqId, "READ_CORE_MEMORY FFC0 32", ReqInfoRMemoryLoRomData, nullptr});
                    break;
                } else {
                    // This should actually still be valid, since you can read wram
                    // but sram will be 'impossible' without the rom mapping
                    makeInfoFail("Can't get ROM info");
                    break;
                }
            }
            setInfoFromRomHeader(QByteArray::fromHex(tList.join()));
            readMemoryHasRomAccess = (romType == HiROM); // see comments in translateAddress()
            state = None;
            emit infoDone(reqId);
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
            doCommandNow({reqId, "READ_CORE_RAM 40FFC0 32", ReqInfoRRAMHiRomData, nullptr});
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
            state = None;
            emit infoDone(reqId);
            break;
        }
        default:
        {
            sDebug() << "OnReadyRead with unexpected state" << state;
            state = None;
            break;
        }
    }

    runCommandQueue(); // send next command if idle
}


void RetroArchHost::onCommandTimerTimeout()
{
    sDebug() << "Command timed out";
    if (state >= ReqInfoVersion && state <= ReqInfoMemory2)
    {
        state = None;
        makeInfoFail("Timeout of a command");
    } else {
        state = None;
        emit commandTimeout(reqId);
    }

    runCommandQueue(); // send next command
}

// ref is https://github.com/tewtal/pusb2snes/blob/master/src/main/python/devices/retroarch.py


int RetroArchHost::translateAddress(unsigned int address)
{
    int addr = static_cast<int>(address);
    if (readMemoryAPI)
    {
        useReadMemoryAPI = true;
        // ROM ACCESS
        if (addr < 0xE00000) {
            if (romType == HiROM && addr < 0x400000) {
                // we can simply access all of HiROM from C00000 to FFFFFF
                return addr + 0xC00000;
            } else if (romType == LoROM) {
                // Not supported. Half of LoROM is unmapped for READ_MEMORY in bsnes-mercury
                return -1;
            } else {
                // ExLo, ExHi untested
                return -1;
            }
        }
        // WRAM
        if (addr >= 0xF50000 && addr <= 0xF70000)
            return 0x7E0000 + addr - 0xF50000;
        // SRAM, this does not work properly with the new api
        // This is a mess anyways xD
        if (addr >= 0xE00000 && addr < 0xF70000)
        {
            useReadMemoryAPI = false;
            //if (!hasRomAccess())
            return addr - 0xE00000 + 0x20000;
            if (romType == LoROM)
                //return addr - 0xE00000 + 0x700000;
                return lorom_sram_pc_to_snes(address - 0xE00000);
            return lorom_sram_pc_to_snes(address - 0xE00000);
        }
        return -1;
    } else {
        useReadMemoryAPI = false;
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

qint64 RetroArchHost::nextId()
{
    if (lastId == std::numeric_limits<qint64>::max()) lastId = -1;
    return ++lastId;
}

qint64 RetroArchHost::queueCommand(QByteArray cmd, State newState, std::function<void(qint64)> sentCallback, qint64 forceId)
{
    qint64 newId = forceId != -1 ? forceId : nextId();
    commandQueue.append({newId, cmd, newState, sentCallback});
    runCommandQueue();
    return newId;
}

void RetroArchHost::runCommandQueue()
{
    while (state == None && !commandQueue.empty()) {
        doCommandNow(commandQueue.front());
        commandQueue.pop_front();
    }
}

void RetroArchHost::doCommandNow(const Command& cmd)
{
    reqId = cmd.id;
    state = cmd.state;
    // if state is None, we don't expect a reply
    if (state != None) commandTimeoutTimer.start();
    writeSocket(cmd.cmd);
    // this is kind of a hack, but we should not run the callback before we returned an id, because it may emit a signal
    auto cb = cmd.sentCallback;
    auto cbId = cmd.id;
    if (cmd.sentCallback) QTimer::singleShot(0, this, [this,cbId,cb]() {
       cb(cbId);
    });
}

void RetroArchHost::writeSocket(QByteArray data)
{
    sDebug() << ">>" << data;
    auto written = socket.writeDatagram(data, m_address, m_port);
    if (written != data.size())
        sDebug() << "only written" << written << "/" << data.size() << "bytes";
}
