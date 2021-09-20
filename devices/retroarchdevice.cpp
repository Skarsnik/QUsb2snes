/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet, Unknownforce351, Thomas Backmark.
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
#include <QUuid>
#include <QVersionNumber>

#include "retroarchdevice.h"
#include "../rommapping/rominfo.h"

Q_LOGGING_CATEGORY(log_retroarch, "RETROARCH")
#define sDebug() qCDebug(log_retroarch)


RetroArchDevice::RetroArchDevice(RetroArchHost* mHost)
{
    host = mHost;
    //hasRomAccess = !info.gameName.isEmpty();
    m_state = READY;
    sDebug() << "Retroarch device created";
    hostName = mHost->name();
    connect(mHost, &RetroArchHost::infoDone, this, &RetroArchDevice::onRHInfoDone);
    connect(mHost, &RetroArchHost::infoFailed, this, &RetroArchDevice::onRHInfoFailled);
    connect(mHost, &RetroArchHost::getMemoryDone, this, &RetroArchDevice::onRHGetMemoryDone);
    connect(mHost, &RetroArchHost::writeMemoryDone, this, &RetroArchDevice::onRHWriteMemoryDone);
    connect(mHost, &RetroArchHost::commandTimeout, this, &RetroArchDevice::onRHCommandTimeout);
    connect(mHost, &RetroArchHost::getMemoryFailed, this, &RetroArchDevice::onRHGetMemoryFailed);
}


QString RetroArchDevice::name() const
{
    return QString("RetroArch %1").arg(hostName);
}


void RetroArchDevice::writeData(QByteArray data)
{
    host->writeMemoryData(data);
}

void RetroArchDevice::infoCommand()
{
    m_state = BUSY;
    reqId = host->getInfos();
}


USB2SnesInfo RetroArchDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data);
    USB2SnesInfo    info;
    m_state = READY;

    if (host->gameTitle().isEmpty() == false)
    {
        info.romPlaying = host->gameTitle();
    } else {
        info.romPlaying = "CAN'T READ ROM INFO";
    }
    info.version = host->version().toString();
    if(host->hasRomAccess() == false)
    {
        info.flags << getFlagString(USB2SnesWS::NO_ROM_READ) << getFlagString(USB2SnesWS::NO_ROM_WRITE);
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
}

void RetroArchDevice::onRHInfoDone(qint64 id)
{
    if (id != reqId)
        return ;
    emit commandFinished();
}

void RetroArchDevice::onRHInfoFailled(qint64 id)
{
    if (id != reqId)
        return ;
    emit protocolError();
}


void RetroArchDevice::onRHGetMemoryDone(qint64 id)
{
    if (id != reqId)
        return;
    sDebug() << "Get memory done";
    m_state = READY;
    emit getDataReceived(host->getMemoryData());
    emit commandFinished();
}

void RetroArchDevice::onRHWriteMemoryDone(qint64 id)
{
    if (id != reqId)
        return;
    sDebug() << "Write memory done";
    m_state = READY;
    emit commandFinished();
}

void RetroArchDevice::onRHCommandTimeout(qint64 id)
{
    Q_UNUSED(id)
    sDebug() << "Command timeout";
    emit protocolError();
    m_state = CLOSED;
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

void RetroArchDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    sDebug() << "GetAddress " << space << addr << size;
    m_state = BUSY;

    if (space != SD2Snes::SNES)
    {
        m_state = CLOSED;
        sDebug() << "Error, address or space incorect";
        close();
        emit protocolError();
        return ;
    }
    reqId = host->getMemory(addr, size);
    if (reqId == -1)
    {
        m_state = CLOSED;
        sDebug() << "Error, address or space incorect";
        close();
        emit protocolError();
        return ;
    }
}

void RetroArchDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    Q_UNUSED(space);
    Q_UNUSED(args);
}

void RetroArchDevice::putAddrCommand(SD2Snes::space space, unsigned int addr0, unsigned int size)
{
    sDebug() << "GetAddress " << space << addr0 << size;
    m_state = BUSY;

    if (space != SD2Snes::SNES)
    {
        m_state = CLOSED;
        sDebug() << "Error, Only SNES space is usable";
        close();
        emit protocolError();
        return ;
    }
    reqId = host->writeMemory(addr0, size);
    if (reqId == -1)
    {
        m_state = CLOSED;
        sDebug() << "Error, address incorect";
        close();
        emit protocolError();
        return ;
    }
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

