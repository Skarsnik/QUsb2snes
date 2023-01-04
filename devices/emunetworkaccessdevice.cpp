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
#include <QCoreApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_emunwaccessdevice, "Emu NWA Device")
#define sDebug() qCDebug(log_emunwaccessdevice)
#define sInfo() qCInfo(log_emunwaccessdevice)

EmuNetworkAccessDevice::EmuNetworkAccessDevice(QString _name, uint port)
{
    myName = _name;
    emu = new EmuNWAccessClient(this);
    emu->connectToHost("127.0.0.1", port);
    connect(emu, &EmuNWAccessClient::connected, this, [=]() {
        sDebug() << "Connected";
        emu->cmdMyNameIs("QUsb2Snes Device");
        currentCmd = USB2SnesWS::Name;
        m_state = READY;
    });
    connect(emu, &EmuNWAccessClient::readyRead, this, &EmuNetworkAccessDevice::onEmuReadyRead, Qt::UniqueConnection);
    connect(emu, &EmuNWAccessClient::disconnected, this, &EmuNetworkAccessDevice::onEmuDisconnected);
    m_state = CLOSED;
    emuVersion.clear();
    uploadedFile = nullptr;
    timerFakeComandFinish.setInterval(10);
    connect(&timerFakeComandFinish, &QTimer::timeout, this, [=] {
        emit commandFinished();
        timerFakeComandFinish.stop();
    });
    doingPutFile = false;
    currentMemorieToWrite = nullptr;
}


void EmuNetworkAccessDevice::onEmuReadyRead()
{
    static int step = 0;
    auto rep = emu->readReply();
    sDebug() << "Reply ready";
    sDebug() << rep;
    if (rep.cmd == "MY_NAME_IS")
    {
        return ;
    }
    switch(currentCmd)
    {
        case USB2SnesWS::Name:
        {
            break;
        }
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
                    if (rep.contains("file"))
                        whatRunning = rep["file"];
                    else {
                        whatRunning = rep["name"];
                    }
                    step = 0;
                    emit commandFinished();
                    return;
                }
            } else {
                sDebug() << "No emu info, checking them";
                if (step == 0)
                {
                    if (rep["state"] == "running")
                        whatRunning = rep["game"];
                    emu->cmdEmulatorInfo();
                    step = 1;
                    return ;
                }
                if (step == 1 && emuVersion.isEmpty())
                {
                    emuVersion = rep["version"];
                    emuName = rep["name"];
                    emu->cmdEmulationStatus();
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
                NWAMemoriesToGet.clear();
                return ;
            }
            if (rep.binary.size() != getAddressSizeRequested)
            {
                emit protocolError();
                NWAMemoriesToGet.clear();
                return ;
            }
            emit getDataReceived(rep.binary);
            if (NWAMemoriesToGet.isEmpty())
            {
                m_state = READY;
                emit commandFinished();
            } else {
                nwaGetMemory(NWAMemoriesToGet.takeFirst());
            }
            break;
        }
        case USB2SnesWS::PutAddress:
        {
            NWAMemoriesToWrite.takeFirst();
            if (NWAMemoriesToWrite.isEmpty())
            {
                m_state = READY;
                emit commandFinished();
                break;
            }
            currentMemorieToWrite = &NWAMemoriesToWrite.first();
            prepareWriteMemory(currentMemorieToWrite->mems);
            if (!cachedData.isEmpty())
            {
                if (currentMemorieToWrite->totalSize <= cachedData.size())
                {
                    emu->cmdCoreWriteMemoryData(cachedData.left(currentMemorieToWrite->totalSize));
                    cachedData = cachedData.mid(currentMemorieToWrite->totalSize);
                } else {
                    emu->cmdCoreWriteMemoryData(cachedData);
                    currentMemorieToWrite->sizeWritten += cachedData.size();
                    cachedData.clear();
                }
            }
            break;
        }
        default:
            break;
    }
}

void EmuNetworkAccessDevice::onEmuDisconnected()
{
    sDebug() << "Emulator disconnected";
    m_state = CLOSED;
    emit closed();
}



EmuNetworkAccessDevice::MemoryAddress    EmuNetworkAccessDevice::sd2snesToDomain(unsigned int sd2snesAddr)
{
    MemoryAddress   toret;
    if (sd2snesAddr >= 0xF50000 && sd2snesAddr < 0xF70000)
    {
        toret.domain = "WRAM";
        toret.offset = sd2snesAddr - 0xF50000;
        return toret;
    }
    if (sd2snesAddr >= 0xE00000)
    {
        toret.domain = "SRAM";
        toret.offset = sd2snesAddr - 0xE00000;
        return toret;
    }
    toret.domain = "CARTROM";
    toret.offset = sd2snesAddr;
    return toret;
}

void EmuNetworkAccessDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    Q_UNUSED(op)
    Q_UNUSED(args)
}


void EmuNetworkAccessDevice::nwaGetMemory(const MemoryAddress& memAdd)
{
    sDebug() << "NWA get Memory single : " << memAdd;
    getAddressSizeRequested = memAdd.size;
    emu->cmdCoreReadMemory(memAdd.domain, memAdd.offset, memAdd.size);
}

void EmuNetworkAccessDevice::nwaGetMemory(const QList<MemoryAddress>& list)
{
    QList<QPair<int, int> >mems;
    const QString domain = list.first().domain;
    getAddressSizeRequested = 0;
    for (const auto& memAdd : list)
    {
        //sDebug() << "NWa get memory multiple : " << memAdd;
        mems.append(QPair<int, int>(memAdd.offset, memAdd.size));
        getAddressSizeRequested += memAdd.size;
    }
    emu->cmdCoreReadMemory(domain, mems);
}

void EmuNetworkAccessDevice::prepareWriteMemory(const QList<MemoryAddress>& list)
{
    QList<QPair<int, int> >mems;
    const QString domain = list.first().domain;
    for (const auto& memAdd : list)
    {
        sDebug() << "NWa put memory multiple : " << memAdd;
        mems.append(QPair<int, int>(memAdd.offset, memAdd.size));
    }
    emu->cmdCoreWriteMemoryPrepare(domain, mems);
}

void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    if (space != SD2Snes::SNES)
    {
        emit protocolError();
        return ;
    }
    m_state = BUSY;
    std::function<void()> F([this, addr, size] {
        currentCmd = USB2SnesWS::GetAddress;
        auto newAddr = sd2snesToDomain(addr);
        newAddr.size = size;
        sDebug() << "Get address" << newAddr.domain << newAddr.offset;
        if (memoryAccess[newAddr.domain].contains("r"))
        {
            nwaGetMemory(newAddr);
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


void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    if (space != SD2Snes::SNES)
    {
        emit protocolError();
        return ;
    }
    m_state = BUSY;
    std::function<void()> F([this, args] {
        currentCmd = USB2SnesWS::GetAddress;
        NWAMemoriesToGet.append(QList<MemoryAddress>());
        QString domain = "";
        for (auto pairing : args)
        {
            auto newAddr = sd2snesToDomain(pairing.first);
            if (!memoryAccess[newAddr.domain].contains("r"))
            {
                emit protocolError();
                return;
            }
            newAddr.size = pairing.second;
            //sDebug() << "Get address" << newAddr;
            if (domain != "" && newAddr.domain != domain)
            {
                NWAMemoriesToGet.append(QList<MemoryAddress>());
            }
            domain = newAddr.domain;
            NWAMemoriesToGet.last().append(newAddr);
        }
        nwaGetMemory(NWAMemoriesToGet.takeFirst());

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

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    // Don't call the "generic" qpair variant, as the size is limited to quint8
    if (space != SD2Snes::SNES)
        emit protocolError();
    m_state = BUSY;
    std::function<void()> F([this, addr, size] {
        currentCmd = USB2SnesWS::PutAddress;
        NWAMemoriesToWrite.append(PutAddressEntry());
        auto newAddr = sd2snesToDomain(addr);
        currentMemorieToWrite = &NWAMemoriesToWrite.last();
        if (!memoryAccess[newAddr.domain].contains("w"))
        {
            emit protocolError();
            NWAMemoriesToWrite.clear();
            return;
        }
        newAddr.size = size;
        currentMemorieToWrite->mems.append(newAddr);
        NWAMemoriesToWrite.last().totalSize += newAddr.size;
        NWAMemoriesToWrite.last().domain = newAddr.domain;
        putAddressTotalSent = 0;
        putAddressTotalSize = size;
        prepareWriteMemory(currentMemorieToWrite->mems);
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

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    if (space != SD2Snes::SNES)
        emit protocolError();
    m_state = BUSY;
    std::function<void()> F([this, args] {
        currentCmd = USB2SnesWS::PutAddress;
        NWAMemoriesToWrite.append(PutAddressEntry());
        QString domain = "";
        sDebug() << "Should be 1" << NWAMemoriesToWrite.size();
        currentMemorieToWrite = &NWAMemoriesToWrite.last();
        for (auto pairing : args)
        {
            auto newAddr = sd2snesToDomain(pairing.first);
            if (!memoryAccess[newAddr.domain].contains("w"))
            {
                emit protocolError();
                NWAMemoriesToWrite.clear();
                return;
            }
            newAddr.size = pairing.second;
            putAddressTotalSize += newAddr.size;
            if (domain != "" && newAddr.domain != domain)
            {
                NWAMemoriesToWrite.append(PutAddressEntry());
                sDebug() << "Adding new putaddressentry" << NWAMemoriesToWrite.size() << NWAMemoriesToWrite.last().totalSize;
            }
            domain = newAddr.domain;
            NWAMemoriesToWrite.last().totalSize += newAddr.size;
            NWAMemoriesToWrite.last().domain = newAddr.domain;
            NWAMemoriesToWrite.last().mems.append(newAddr);
        }
        putAddressTotalSent = 0;
        prepareWriteMemory(currentMemorieToWrite->mems);
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

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    QList<QPair<unsigned int, quint8> > plop;
    plop.append(QPair<unsigned int, quint8>(addr, size));
    putAddrCommand(space, plop);
}

void EmuNetworkAccessDevice::infoCommand()
{
    m_state = BUSY;
    std::function<void()> F([this] {
        currentCmd = USB2SnesWS::Info;
        sDebug() << "Info command";
        emu->cmdEmulationStatus();
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
    if (doingPutFile)
    {
        uploadedFileSizeWritten += data.size();
        uploadedFile->write(data);
        if (uploadedFileSize == uploadedFileSizeWritten)
        {
            doingPutFile = false;
            uploadedFile->close();
            delete uploadedFile;
            emit commandFinished();
        }
        return ;
    }
    putAddressTotalSent += data.size();
    sDebug() << "Total Sent : " << putAddressTotalSent << "Total Size" << putAddressTotalSize;
    if (putAddressTotalSent > putAddressTotalSize)
    {
        emit protocolError();
        return;
    }
    // If we get data but the emu still has not confirm the previous write
    if (currentMemorieToWrite->totalSize == currentMemorieToWrite->sizeWritten && NWAMemoriesToWrite.size() > 1)
    {
        cachedData += data;
        return ;
    }
    int remainingWrite = currentMemorieToWrite->totalSize - currentMemorieToWrite->sizeWritten;
    sDebug() << "Remaining to write : " << remainingWrite;
    if (data.size() < remainingWrite)
    {
        emu->cmdCoreWriteMemoryData(data);
        currentMemorieToWrite->sizeWritten += data.size();
    }
    if (data.size() >= remainingWrite)
    {
        emu->cmdCoreWriteMemoryData(data.left(remainingWrite));
        currentMemorieToWrite->sizeWritten += remainingWrite;
        if (data.size() > remainingWrite)
        {
            cachedData += data.mid(remainingWrite);
        }
    }
}

QString EmuNetworkAccessDevice::name() const
{
    return myName;
}

bool EmuNetworkAccessDevice::hasFileCommands()
{
    return true;
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

bool EmuNetworkAccessDevice::open()
{
    //m_state = READY;
    return emu->waitForConnected(20);
    //return true;
}

void EmuNetworkAccessDevice::close()
{
    if (m_state != CLOSED)
        emu->disconnectFromHost();
    m_state = CLOSED;
}

QDebug operator<<(QDebug debug, const EmuNetworkAccessDevice::MemoryAddress &ma)
{
    debug << ma.domain << " : " << ma.offset << ":" <<ma.size;
    return debug;
}

void EmuNetworkAccessDevice::putFile(QByteArray name, unsigned int size)
{
    doingPutFile = true;
    uploadedFileName = name;
    uploadedFile = LocalStorage::prepareWriteFile(name, size);
    uploadedFileSize = size;
    uploadedFileSizeWritten = 0;
    if (!uploadedFile->isOpen())
    {
        doingPutFile = false;
        delete uploadedFile;
        uploadedFile = nullptr;
        emit protocolError();
    }
}


void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    bool success = true;
    switch (op)
    {
    case SD2Snes::opcode::GET:
    {
        QByteArray data = LocalStorage::getFile(args.at(0));
        if (data.isEmpty())
        {
            success = false;
            break;
        }
        emit sizeGet(data.size());
        emit getDataReceived(data);
        break;
    }
    case SD2Snes::opcode::RM:
    {
        success = LocalStorage::remove(args.at(0));
        break;
    }
    case SD2Snes::opcode::LS:
    {
        fileInfos = LocalStorage::list(args.at(0));
        if (fileInfos.isEmpty())
        {
            success = false;
            break;
        }
        break;
    }
    case SD2Snes::opcode::MV:
    {
        success = LocalStorage::rename(args.at(0), args.at(1));
        break;
    }
    case SD2Snes::opcode::MKDIR:
    {
        success = LocalStorage::makeDir(args.at(0));
        break;
    }
    default:
    {
        sDebug() << "Asked a bad file command";
        emit protocolError();
        return ;
    }
    }
    if (success) {
        timerFakeComandFinish.start();
    } else {
        emit protocolError();
    }
}

void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    QVector<QByteArray> p;
    p.append(args);
    fileCommand(op, p);
}

QList<ADevice::FileInfos> EmuNetworkAccessDevice::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI);
    QList<ADevice::FileInfos> toret;
    for (const auto& finfo : qAsConst(fileInfos))
    {
        ADevice::FileInfos fi;
        fi.name = finfo.name;
        fi.type = static_cast<SD2Snes::file_type>(finfo.type);
        toret.append(fi);
    }
    fileInfos.clear();
    return toret;
}


bool EmuNetworkAccessDevice::hasVariaditeCommands()
{
    return true;
}
