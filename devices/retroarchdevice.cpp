#include <QEventLoop>
#include <QLoggingCategory>
#include <QUuid>

#include "retroarchdevice.h"
#include "../rommapping/rominfo.h"

Q_LOGGING_CATEGORY(log_retroarch, "RETROARCH")
#define sDebug() qCDebug(log_retroarch)

RetroArchDevice::RetroArchDevice(QUdpSocket* sock, QString raVersion, QString gameName, int bSize, bool snesMemoryMap, bool snesLoromMap)
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
    hasSnesMemoryMap = snesMemoryMap;
    hasSnesLoromMap = snesLoromMap;
    checkingInfo = false;
    m_raVersion = raVersion;
    m_gameName = gameName;
    blockSize = static_cast<unsigned int>(bSize);
    m_uuid = QUuid::createUuid().toString(QUuid::Id128).left(24);
    c_rom_infos = nullptr;
}


QString RetroArchDevice::name() const
{
    return QString("RetroArch %1").arg(m_uuid);
}


USB2SnesInfo RetroArchDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data);
    USB2SnesInfo    info;

    info.romPlaying = c_rom_infos->title;
    hasSnesLoromMap = c_rom_infos->type == LoROM;
    info.version = m_raVersion;
    if(!hasSnesMemoryMap)
    {
        info.flags << getFlagString(USB2SnesWS::NO_ROM_READ);
    } else {
        info.flags << "SNES_MEMORY_MAP";

        if(hasSnesLoromMap)
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
    static rom_type infoRequestType = HiROM;

    QByteArray data = m_sock->readAll();
    sDebug() << "<<" << data;
    if (data.isEmpty())
    {
        emit closed();
        return ;
    }
    QList<QByteArray> tList = data.trimmed().split(' ');
    //sDebug() << tList;

    if(checkingInfo)
    {
        checkingInfo = false;
        //TODO emit an error
        if(tList.at(2) == "-1")
        {
            hasSnesMemoryMap = false;
            m_state = READY;
            emit commandFinished();
            return ;
        } else {
            if (c_rom_infos != nullptr)
                free(c_rom_infos);
            tList.removeFirst();tList.removeFirst();
            c_rom_infos = get_rom_info(QByteArray::fromHex(tList.join()));
            if (rom_info_make_sense(c_rom_infos, infoRequestType) || infoRequestType == LoROM)
            {
                infoRequestType = HiROM;
                hasSnesMemoryMap = true;
                hasSnesLoromMap = c_rom_infos->type;
                m_state = READY;
                emit commandFinished();
                return ;
            }

            // Checking LoROM
            auto tmpData = QByteArray("READ_CORE_RAM ") + QByteArray::number(0x7FC0 + (0x8000 * ((0x7FC0 + 0x8000) / 0x8000))) + " 32";
            m_sock->write(tmpData);
            infoRequestType = LoROM;
            return ;
        }
    }

    if (tList.at(2) != "-1")
    {
        tList = tList.mid(2);
        data = tList.join();
        emit getDataReceived(QByteArray::fromHex(data));
    } else {
        sDebug() << "Not giving data : sending" << lastRCRSize << "bytes";
        emit getDataReceived(QByteArray(static_cast<int>(lastRCRSize), 0));
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

            if(hasSnesLoromMap && (addrBigGet < 0x700000 || addrBigGet >= 0x800000))
            {
                if(((addrBigGet&0xFFFF) + sizePrevBigGet) > 0xFFFF)
                {
                    addrBigGet += 0x8000;
                }

                if(((addrBigGet + mSize)&0xFFFF) < 0x8000)
                {
                    mSize = 0x10000 - (addrBigGet&0xFFFF);
                }
            }

            sizePrevBigGet = mSize;
            sizeRequested += mSize;
            read_core_ram(addrBigGet, mSize);
         }
         return;
    }
    emit commandFinished();
    m_state = READY;
}

void RetroArchDevice::read_core_ram(unsigned int addr, unsigned int size)
{
    QByteArray data = "READ_CORE_RAM " + QByteArray::number(addr, 16) + " " + QByteArray::number(size);
    lastRCRSize = size;
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

int RetroArchDevice::addr_to_addr(unsigned int addr)
{
    if(!hasSnesMemoryMap)
    {
        if (addr >= 0xF50000 && addr <= 0xF70000)
            addr -= 0xF50000;
        else {
            if (addr >= 0xE00000)
                addr = addr - 0xE00000 + 0x20000;
            else
                return -1;
        }
    } else {
        if(addr >= 0xF50000 && addr <= 0xF70000)
        {
            addr -= 0x770000;
        }
        else if(addr >= 0xE00000 && addr <= 0xE10000)
        {
            if(hasSnesLoromMap)
            {
                addr -= 0x700000;
            } else {
                addr -= 0x3FA000;
            }
        }
        else if(addr < 0x700000)
        {
            if(hasSnesLoromMap)
            {
                addr = (addr + (0x8000 * ((addr+0x8000)/0x8000)));
            } else {
                addr = 0x400000;
            }
        }
    }
    return static_cast<int>(addr);
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
        emit getDataReceived(QByteArray(static_cast<int>(size), 0));
        //m_timer->start();
        sDebug() << "GetAddress finished";
        m_state = READY;
        emit commandFinished();
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

    if (hasSnesLoromMap && (newAddr < 0x700000 || newAddr >= 0x800000) && ((newAddr&0xFFFF) >= 0x8000))
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
    }

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
    sDebug() << "Info command: Checking core memory map status";
    auto tmpData = "READ_CORE_RAM 40FFC0 32";
    m_sock->write(tmpData);
    checkingInfo = true;
}

void RetroArchDevice::writeData(QByteArray data)
{
    sizeWritten += static_cast<unsigned int>(data.size());
    dataToWrite.append(data.toHex(' '));
    if(sizeWritten >= sizePut)
    {
        m_sock->write(dataToWrite);
        sDebug() << "Write finished";
        m_state = READY;
        emit commandFinished();
    }
}
