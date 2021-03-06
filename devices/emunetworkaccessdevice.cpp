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



#include "emunetworkaccessdevice.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_emunwaccessdevice, "Emu NWA Device")
#define sDebug() qCDebug(log_emunwaccessdevice)
#define sInfo() qCInfo(log_emunwaccessdevice)

EmuNetworkAccessDevice::EmuNetworkAccessDevice(QString _name)
{
    myName = _name;
    emu = new EmuNWAccessClient(this);
    emu->connectToHost("127.0.0.1", 65400);
    connect(emu, &EmuNWAccessClient::connected, this, [=]() {
        sDebug() << "Connected";
        m_state = READY;
    });
    connect(emu, &EmuNWAccessClient::readyRead, this, &EmuNetworkAccessDevice::onEmuReadyRead, Qt::UniqueConnection);
    connect(emu, &EmuNWAccessClient::disconnected, this, [=]()
    {
       m_state = CLOSED;
       emit closed();
    });
    m_state = CLOSED;
    emuVersion.clear();
}


void EmuNetworkAccessDevice::onEmuReadyRead()
{
    static int step = 0;
    auto rep = emu->readReply();
    sDebug() << "Reply ready";
    sDebug() << rep;
    switch(currentCmd)
    {
        // This is actually memory access request
        case USB2SnesWS::Attach:
        {
            auto memAccess = rep.toMapList();
            for (auto& ma : memAccess)
                memoryAccess[ma["name"]] = ma["access"];
            afterMemoryAccess();
            afterMemoryAccess = [](){;};
            break;
        }
        case USB2SnesWS::Info:
        {
            if (!emuVersion.isEmpty())
            {
                if (step == 0) {
                    if (rep["state"] == "no_game")
                    {
                        whatRunning = "";
                        emit commandFinished();
                        return ;
                    } else {
                        step = 1;
                        emu->cmdGameInfo();
                    }
                } else {
                    whatRunning = rep["file"];
                    step = 0;
                    emit commandFinished();
                    return;
                }
            } else {
                sDebug() << "No emu info, checking them";
                if (step == 0)
                {
                    emu->cmdEmuInfo();
                    step = 1;
                    return ;
                }
                if (step == 1 && emuVersion.isEmpty())
                {
                    emuVersion = rep["version"];
                    emuName = rep["name"];
                    emu->cmdEmuStatus();
                    step = 0;
                }
            }
            break;
        }
        case USB2SnesWS::GetAddress:
        {
            if (rep.isError)
            {
                emit protocolError();
                return ;
            }
            if (rep.binary.size() != getAddressSizeRequested)
            {
                emit protocolError();
                return ;
            }
            emit getDataReceived(rep.binary);
            m_state = READY;
            emit commandFinished();
            break;
        }
        case USB2SnesWS::PutAddress:
        {
            m_state = READY;
            emit commandFinished();
            break;
        }
        default:
            break;
    }
}



QPair<QString, unsigned int>    EmuNetworkAccessDevice::sd2snesToDomain(unsigned int sd2snesAddr)
{
    QPair<QString, unsigned int> toret;
    if (sd2snesAddr >= 0xF50000 && sd2snesAddr < 0xF70000)
    {
        toret.first = "WRAM";
        toret.second = sd2snesAddr - 0xF50000;
        return toret;
    }
    if (sd2snesAddr >= 0xE00000)
    {
        toret.first = "SRAM";
        toret.second = sd2snesAddr - 0xE00000;
        return toret;
    }
    toret.first = "CARTROM";
    toret.second = sd2snesAddr;
    return toret;
}

void EmuNetworkAccessDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}

void EmuNetworkAccessDevice::putFile(QByteArray name, unsigned int size)
{
    Q_UNUSED(name)
    Q_UNUSED(size)
}

void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    if (space != SD2Snes::SNES)
        emit protocolError();
    m_state = BUSY;
    std::function<void()> F([this, addr, size] {
        currentCmd = USB2SnesWS::GetAddress;
        auto newAddr = sd2snesToDomain(addr);
        sDebug() << "Get address" << newAddr;
        getAddressSizeRequested = size;
        if (memoryAccess[newAddr.first].contains("r"))
        {
            emu->cmdCoreReadMemory(newAddr.first, newAddr.second, size);
        } else {
            emit protocolError();
        }
    });
    if (!memoryAccess.contains("WRAM"))
    {
        currentCmd = USB2SnesWS::Attach;
        emu->cmdCoreMemories();
        afterMemoryAccess = F;
    } else {
        F();
    }
}


// TODO later, we don't support it for now
void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    putAddrCommand(space, 0, addr, size);
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space)
    Q_UNUSED(args)
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    if (space != SD2Snes::SNES)
        emit protocolError();
    m_state = BUSY;
    std::function<void()> F([this, addr, size] {
        currentCmd = USB2SnesWS::PutAddress;
        auto newAddr = sd2snesToDomain(addr);
        sDebug() << "Put address" << newAddr;
        getAddressSizeRequested = size;
        if (memoryAccess[newAddr.first].contains("r"))
        {
            putAddressSize = size;
            putAddressSizeSent = 0;
            emu->cmdCoreWriteMemoryPrepare(newAddr.first, QString::number(newAddr.second), size);
        } else {
            emit protocolError();
        }
    });
    if (!memoryAccess.contains("WRAM"))
    {
        currentCmd = USB2SnesWS::Attach;
        emu->cmdCoreMemories();
        afterMemoryAccess = F;
    } else {
        F();
    }
}

void EmuNetworkAccessDevice::infoCommand()
{
    m_state = BUSY;
    std::function<void()> F([this] {
        currentCmd = USB2SnesWS::Info;
        sDebug() << "Info command";
        emu->cmdEmuStatus();
    });
    if (memoryAccess.isEmpty())
    {
        currentCmd = USB2SnesWS::Attach;
        emu->cmdCoreMemories();
        afterMemoryAccess = F;
    } else {
        F();
    }

}

void EmuNetworkAccessDevice::writeData(QByteArray data)
{
    putAddressSizeSent += data.size();
    if (putAddressSizeSent > putAddressSize)
        emit protocolError();
    emu->cmdCoreWriteMemoryData(data);
}

QString EmuNetworkAccessDevice::name() const
{
    return myName;
}

bool EmuNetworkAccessDevice::hasFileCommands()
{
    return false;
}

bool EmuNetworkAccessDevice::hasControlCommands()
{
    return false;
}

USB2SnesInfo EmuNetworkAccessDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data);
    USB2SnesInfo toret;
    toret.version = emuVersion;
    toret.deviceName = "NWAccess";
    if (whatRunning == "")
        toret.romPlaying = "/bin/menu.bin"; // Fall back to menu
    else
        toret.romPlaying = whatRunning;
    if (memoryAccess.contains("CARTROM") && !memoryAccess["CARTROM"].contains("w"))
        toret.flags.append(getFlagString(USB2SnesWS::NO_ROM_WRITE));
    if (!memoryAccess.contains("CARTROM"))
        toret.flags.append(getFlagString(USB2SnesWS::NO_ROM_READ));
    m_state = READY;
    return toret;
}

QList<ADevice::FileInfos> EmuNetworkAccessDevice::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI);
    return QList<ADevice::FileInfos>();
}

bool EmuNetworkAccessDevice::open()
{
    m_state = READY;
    return true;
}

void EmuNetworkAccessDevice::close()
{
    if (m_state != CLOSED)
        emu->disconnectFromHost();
}


void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
}

void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
}
