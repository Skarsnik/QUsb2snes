#include "luabridgedevice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include "../rommapping/rommapping.h"

Q_LOGGING_CATEGORY(log_luaBridgeDevice, "LUABridgeDevice")
#define sDebug() qCDebug(log_luaBridgeDevice)

LuaBridgeDevice::LuaBridgeDevice(QTcpSocket* sock, QString name)
{
    timer.setInterval(3);
    timer.setSingleShot(true);
    m_state = CLOSED;
    m_socket = sock;
    m_name = name;
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

void    LuaBridgeDevice::getRomMapping()
{
    sDebug() << "Trying to get ROM mapping";
    if (bizhawk)
        m_socket->write("Read|" + QByteArray::number(0x7FC0) + "|22|CARTROM\n");
    else
        m_socket->write("Read|" +  QByteArray::number(0x00FFC0) + "|22\n");
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
        sDebug() << data;
        if (data.at(21) & 0x01)
            romMapping = LoROM;
        else
            romMapping = HiROM;
    }
    sDebug() << "ROM is " << ((romMapping == LoROM) ? "LoROM" : "HiROM");
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

unsigned int LuaBridgeDevice::getSnes9xAddress(unsigned int addr)
{
    if (addr >= 0xF50000 && addr < 0xF70000)
        return addr - 0xF50000 + 0x7E0000;
    if (addr >= 0xE00000)
        return static_cast<unsigned int>(rommapping_sram_pc_to_snes(addr - 0xE00000, romMapping, false));
    return static_cast<unsigned int>(rommapping_pc_to_snes(addr, romMapping, false));
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
    m_state = BUSY;
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    Q_UNUSED(space)
    m_state = BUSY;
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
    m_state = BUSY;
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
            toSend += "|" + QByteArray::number(static_cast<unsigned char>(data.at(i)));
        toSend += "\n";
        sDebug() << ">>" << toSend;
        m_socket->write(toSend);
        emit commandFinished();
        receivedSize = 0;
        received.clear();
        m_state = READY;
    }
}

QString LuaBridgeDevice::name() const
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
    info.romPlaying = "No Info";
    info.version = "1.0.0";
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
    m_state = READY;
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
        QJsonDocument   jdoc = QJsonDocument::fromJson(dataRead);
        QJsonObject     job = jdoc.object();
        QJsonArray      aData = job["data"].toArray();
        //sDebug() << aData;
        data.clear();
        foreach (QVariant v, aData.toVariantList())
        {
            data.append(static_cast<char>(v.toInt()));
        }
        sDebug() << data;
        m_state = READY;
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
    m_state = READY;
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
