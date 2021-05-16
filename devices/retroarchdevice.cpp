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

}


QString RetroArchDevice::name() const
{
    return QString("RetroArch %1").arg(hostName);
}


void RetroArchDevice::writeData(QByteArray data)
{
    /*sizeWritten += static_cast<unsigned int>(data.size());
    dataToWrite.append(data.toHex(' '));
    if(sizeWritten >= sizePut)
    {
        sDebug() << ">>" << dataToWrite;
        m_sock->write(dataToWrite);
        m_sock->waitForBytesWritten(20);
        sDebug() << "Write finished";
        m_state = READY;
        m_timeout_timer->stop();
        emit commandFinished();
    }*/
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
        info.flags << getFlagString(USB2SnesWS::NO_ROM_READ);
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


void RetroArchDevice::timedCommandDone()
{
    sDebug() << "Fake cmd finished";
    m_state = READY;
    emit commandFinished();
}

void RetroArchDevice::commandTimeout()
{
    sDebug() << "Command timed out, closing the connection";
    close();
    emit closed();
}

void RetroArchDevice::onRHGetMemoryDone(qint64 id)
{
    sDebug() << "Get memory done";
    if (id != reqId)
        return;
    m_state = READY;
    emit getDataReceived(host->getMemoryData());
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
    sizePut = size;
    sizeWritten = 0;
    unsigned int addr = addr0;
    m_state = BUSY;
    /*int newAddr = addr_to_addr(addr);
    if (space != SD2Snes::space::SNES || newAddr == -1)
    {
        m_state = CLOSED;
        sDebug() << "Error, address or space incorect";
        emit protocolError();
        return ;
    }
    sDebug() << "WRITING TO RAM/SRAM" << newAddr;
    dataToWrite = "WRITE_CORE_RAM " + QByteArray::number(newAddr, 16) + " ";
    //m_timeout_timer->start();
    */
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

