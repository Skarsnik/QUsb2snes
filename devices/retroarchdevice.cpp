#include <QEventLoop>
#include <QLoggingCategory>
#include <QUuid>
#include <QVersionNumber>

#include "retroarchdevice.h"
#include "../rommapping/rominfo.h"

Q_LOGGING_CATEGORY(log_retroarch, "RETROARCH")
#define sDebug() qCDebug(log_retroarch)

/*
RetroArchDevice::RetroArchDevice(QUdpSocket *sock, QString raVersion, int bSize)
{
    create(sock, raVersion, bSize);
    hasRomAccess = false;
}

RetroArchDevice::RetroArchDevice(QUdpSocket *sock, QString raVersion, int bSize, QString gameName, rom_type pRomType)
{
    create(sock, raVersion, bSize);
    m_gameName = gameName;
    romType = pRomType;
    hasRomAccess = true;
}
*/

RetroArchDevice::RetroArchDevice(QUdpSocket *sock, RetroArchInfos info, QString name)
{
    create(sock, info.version, info.blockSize);
    hostName = name;
    m_gameName = info.gameName;
    romType = info.romType;
    hasRomAccess = !info.gameName.isEmpty();
}


RetroArchInfos RetroArchDevice::getRetroArchInfos(QUdpSocket *sock)
{
    RetroArchInfos infos;

    sock->write("VERSION");
    sock->waitForReadyRead(100);
    if(sock->hasPendingDatagrams())
    {
        infos.version = sock->readAll().trimmed();
        if (infos.version == "")
        {
            infos.error = tr("RetroArch - Probably not running");
            return infos;
        }
        sDebug() << "Received RA version : " << infos.version;
    } else {
        infos.error = tr("RetroArch - Did not get a VERSION response.");
        return infos;
    }
    sDebug() << "Checking if something is running (read core 0 1)";
    sock->write("READ_CORE_RAM 0 1");
    sock->waitForReadyRead(100);
    QByteArray data;
    if (!sock->hasPendingDatagrams())
    {
        infos.error = tr("RetroArch - No game is running or core does not support memory read.");
        return infos;
    }
    data = sock->readAll();
    sDebug() << "<<" << data;
    if (data == "READ_CORE_RAM 0 -1\n")
    {
        infos.error = tr("RetroArch - No game is running or core does not support memory read.");
        return infos;
    }

    /*
     * alttp (lorom), sm (hirom), smz3 combo (exhirom)
     * Reading FFC0
     * Snes9x core
     *  loRom : all zeros
     *  hiRom : all 55
     *  exHiRom : all 55
     * BSnes-mercury
     *  loRom : correct data
     *  hiRom : correct data
     *  exHirom : incorrect data
     * --
     * Reading 40FFC0
     * Snes9x core
     *  loRom : -1
     *  hiRom : -1
     *  exHiRom : -1
     * BSnes-mercury
     *  loRom : correct data
     *  hiRom : correct data
     *  exHirom : correct data
     * */
    sDebug() << "Trying to get rom header";
    sock->write(QByteArray("READ_CORE_RAM 40FFC0 32"));
    sock->waitForReadyRead(100);
    if (sock->hasPendingDatagrams())
    {
        data = sock->readAll();
        sDebug() << "<<" << data;
        QList<QByteArray> tList = data.trimmed().split(' ');
        infos.blockSize = 78;
        if (QVersionNumber::fromString(infos.version) >= QVersionNumber::fromString("1.7.7"))
            infos.blockSize = 2000;
        // This is likely snes9x core
        if (tList.at(2) == "-1")
        {
            return infos;
        } else {
            tList.removeFirst();
            tList.removeFirst();
            struct rom_infos* rInfos = get_rom_info(QByteArray::fromHex(tList.join()).data());
            sDebug() << rInfos->title;
            infos.gameName = QString(rInfos->title);
            infos.romType = rInfos->type;
            free(rInfos);
        }
    } else {
        infos.error = tr("RetroArch - Did not get a response to reading rominfos.");
    }
    return infos;
}


void    RetroArchDevice::create(QUdpSocket* sock, QString raVersion, int bSize)
{
    m_sock = sock;
    m_state = READY;
    sDebug() << "Retroarch device created";
    m_timer = new QTimer();
    m_timer->setInterval(5);
    m_timer->setSingleShot(true);
    dataRead = QByteArray();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timedCommandDone()));
    connect(m_sock, SIGNAL(readyRead()), this, SLOT(onUdpReadyRead()));
    bigGet = false;
    checkingRetroarch = false;
    checkingInfo = false;
    m_raVersion = raVersion;
    blockSize = static_cast<unsigned int>(bSize);
    m_uuid = QUuid::createUuid().toString(QUuid::Id128).left(24);
    c_rom_infos = nullptr;
}


QString RetroArchDevice::name() const
{
    return QString("RetroArch %1").arg(hostName);
}


USB2SnesInfo RetroArchDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data);
    USB2SnesInfo    info;

    if (c_rom_infos != nullptr)
    {
        info.romPlaying = c_rom_infos->title;
    } else {
        info.romPlaying = "CAN'T READ ROM INFO";
    }
    info.version = m_raVersion;
    if(!hasRomAccess)
    {
        info.flags << getFlagString(USB2SnesWS::NO_ROM_READ);
    } else {
        info.flags << "SNES_MEMORY_MAP";

        if(romType == LoROM)
        {
            info.flags << "SNES_LOROM";
        } else {
            info.flags << "SNES_HIROM";
        }
    }
    info.deviceName = "RetroArch";
    info.flags << getFlagString(USB2SnesWS::NO_CONTROL_CMD);
    info.flags << getFlagString(USB2SnesWS::NO_FILE_CMD);
    return info;
}

QList<ADevice::FileInfos> RetroArchDevice::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI);
    return QList<ADevice::FileInfos>();
}


bool RetroArchDevice::open()
{
    return m_state != CLOSED;
}

void RetroArchDevice::close()
{
    m_state = CLOSED;
    m_sock->close();
}

void RetroArchDevice::onUdpReadyRead()
{
    QByteArray data = m_sock->readAll();
    sDebug() << "<<" << data;
    if (data.isEmpty())
    {
        emit closed();
        return ;
    }
    QList<QByteArray> tList = data.trimmed().split(' ');
    //sDebug() << tList;

    if (checkingInfo)
    {
        checkingInfo = false;
        if(tList.at(2) == "-1")
        {
            c_rom_infos = nullptr;
            hasRomAccess = false;
            m_state = READY;
            emit commandFinished();
            return ;
        } else {
            if (c_rom_infos != nullptr)
                free(c_rom_infos);
            tList.removeFirst();tList.removeFirst();
            c_rom_infos = get_rom_info(QByteArray::fromHex(tList.join()));
            hasRomAccess = true;
            romType = c_rom_infos->type;
            m_state = READY;
            emit commandFinished();
            return ;
        }
    }

    if (tList.at(2) != "-1")
    {
        tList = tList.mid(2);
        data = tList.join();
        emit getDataReceived(QByteArray::fromHex(data));
    } else {
        emit protocolError();
        return ;
    }

    if (bigGet)
    {
        if (sizeRequested == sizeBigGet)
        {
            bigGet = false;
            sizeBigGet = 0;
            sizeRequested = 0;
            addrBigGet = 0;
            m_state  = READY;
            emit commandFinished();
        } else {
            unsigned int mSize = 0;
            if (sizeRequested + blockSize <= sizeBigGet)
                mSize = blockSize;
            else
                mSize = sizeBigGet - sizeRequested;


            addrBigGet += sizePrevBigGet;

            /*if(hasSnesLoromMap && (addrBigGet < 0x700000 || addrBigGet >= 0x800000))
            {
                if(((addrBigGet&0xFFFF) + sizePrevBigGet) > 0xFFFF)
                {
                    addrBigGet += 0x8000;
                }

                if(((addrBigGet + mSize)&0xFFFF) < 0x8000)
                {
                    mSize = 0x10000 - (addrBigGet&0xFFFF);
                }
            }*/

            sizePrevBigGet = mSize;
            sizeRequested += mSize;
            read_core_ram(addrBigGet, mSize);
         }
         return;
    }
    m_state = READY;
    emit commandFinished();
}

void RetroArchDevice::read_core_ram(unsigned int addr, unsigned int size)
{
    QByteArray data = "READ_CORE_RAM " + QByteArray::number(addr, 16) + " " + QByteArray::number(size);
    lastRCRSize = size;
    sDebug() << ">>" << data;
    m_sock->write(data);
}

void RetroArchDevice::timedCommandDone()
{
    sDebug() << "Fake cmd finished";
    m_state = READY;
    emit commandFinished();
}

bool RetroArchDevice::hasFileCommands()
{
    return false;
}

bool RetroArchDevice::hasControlCommands()
{
    return false;
}

void RetroArchDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    Q_UNUSED(op);
    Q_UNUSED(args);
}

void RetroArchDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op);
    Q_UNUSED(args);

}

void RetroArchDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op);
    Q_UNUSED(args);

}

void RetroArchDevice::putFile(QByteArray name, unsigned int size)
{
    Q_UNUSED(name);
    Q_UNUSED(size);
}

// ref is https://github.com/tewtal/pusb2snes/blob/master/src/main/python/devices/retroarch.py

int RetroArchDevice::addr_to_addr(unsigned int addr2)
{
    int addr = static_cast<int>(addr2);
    // Rom access
    if (addr < 0xE00000)
    {
        if (!hasRomAccess)
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
        if (hasRomAccess) {
            return addr - 0xF50000 + 0x7E0000;
        } else {
            return addr - 0xF50000;
        }
    }
    if (addr >= 0xE00000)
    {
        if (!hasRomAccess)
            return addr - 0xE00000 + 0x20000;
        if (romType == LoROM)
            return addr - 0xE00000 + 0x700000;
        return lorom_sram_pc_to_snes(addr2 - 0xE00000);
    }
}

void RetroArchDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    sDebug() << "GetAddress " << space << addr << size;
    m_state = BUSY;

    if (space != SD2Snes::SNES)
        return;

    auto newAddr = addr_to_addr(addr);
    if (newAddr == -1)
    {
        m_state = CLOSED;
        emit protocolError();
        return ;
    }

    if (size > blockSize)
    {
        bigGet = true;
        sizeBigGet = size;
        sizeRequested = blockSize;
        size = blockSize;
        sizePrevBigGet = blockSize;
        addrBigGet = static_cast<unsigned int>(newAddr);
    } else {
        bigGet = false;
        sizeBigGet = 0;
        addrBigGet = 0;
    }

    /*if (hasSnesLoromMap && (newAddr < 0x700000 || newAddr >= 0x800000) && ((newAddr&0xFFFF) >= 0x8000))
    {
        if(((static_cast<unsigned int>(newAddr)+size)&0xFFFF) < 0x8000)
        {
            bigGet = true;
            sizeBigGet = size;
            sizeRequested = 0x10000 - (newAddr&0xFFFF);
            size = sizeRequested;
            sizePrevBigGet = size;
            addrBigGet = static_cast<unsigned int>(newAddr);
        }
    }*/

    read_core_ram(static_cast<unsigned int>(newAddr), size);
}

void RetroArchDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space);
    Q_UNUSED(args);
}

void RetroArchDevice::putAddrCommand(SD2Snes::space space, unsigned int addr0, unsigned int size)
{
    sizePut = size;
    sizeWritten = 0;
    unsigned int addr = addr0;
    m_state = BUSY;

    if (space != SD2Snes::space::SNES)
        return;

    int newAddr = addr_to_addr(addr);

    if (newAddr == -1)
        return;

    sDebug() << "WRITING TO RAM/SRAM" << newAddr;
    dataToWrite = "WRITE_CORE_RAM " + QByteArray::number(newAddr, 16) + " ";
}

void RetroArchDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space);
    Q_UNUSED(args);
}

void RetroArchDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    Q_UNUSED(flags);
    putAddrCommand(space, addr, size);
}

void RetroArchDevice::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
    Q_UNUSED(opcode);
    Q_UNUSED(space);
    Q_UNUSED(flags);
    Q_UNUSED(arg);
    Q_UNUSED(arg2);
}

void RetroArchDevice::infoCommand()
{
    m_state = BUSY;
    sDebug() << "Info command: Checking core memory map status";
    auto tmpData = "READ_CORE_RAM 40FFC0 32";
    sDebug() << ">> " << tmpData;
    m_sock->write(tmpData);
    checkingInfo = true;
}

void RetroArchDevice::writeData(QByteArray data)
{
    sizeWritten += static_cast<unsigned int>(data.size());
    dataToWrite.append(data.toHex(' '));
    if(sizeWritten >= sizePut)
    {
        sDebug() << ">>" << dataToWrite;
        m_sock->write(dataToWrite);
        m_sock->waitForBytesWritten(20);
        sDebug() << "Write finished";
        m_state = READY;
        emit commandFinished();
    }
}
