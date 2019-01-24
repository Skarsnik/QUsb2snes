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
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
    m_name = name;
    sDebug() << "LUA bridge device created";

}

static unsigned int usb2snes_addr_to_snes(unsigned int addr)
{
    if (addr >= 0xF50000 && addr < 0xF70000)
        return addr - 0xF50000 + 0x7E0000;
    if (addr >= 0xE00000)
        return rommapping_sram_pc_to_snes(addr - 0xE00000, LoROM, false);
    return rommapping_pc_to_snes(addr, LoROM, false);
}

void LuaBridgeDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    QByteArray toWrite = "Read|" + QByteArray::number(usb2snes_addr_to_snes(addr)) + "|" + QByteArray::number(size) + "\n";
    sDebug() << ">>" << toWrite;
    sDebug() << "Writen" << m_socket->write(toWrite) << "Bytes";
    m_state = BUSY;
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    m_state = BUSY;
    putAddr = addr;
    putSize = size;
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}

void LuaBridgeDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
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
    receivedSize += data.size();
    received += data;
    if (receivedSize == putSize)
    {
        QByteArray toSend = "Write|" + QByteArray::number(usb2snes_addr_to_snes(putAddr));
        for (unsigned int i = 0; i < received.size(); i++)
            toSend += "|" + QByteArray::number((unsigned char) data.at(i));
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
    USB2SnesInfo info;
    info.romPlaying = "No Info";
    info.version = "1.0.0";
    return info;
}

QList<ADevice::FileInfos> LuaBridgeDevice::parseLSCommand(QByteArray &dataI)
{
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

    //sDebug() << "<<" << data;
    if (dataRead.right(1) == "\n")
    {
        QJsonDocument   jdoc = QJsonDocument::fromJson(dataRead);
        QJsonObject     job = jdoc.object();
        QJsonArray      aData = job["data"].toArray();
        //sDebug() << aData;
        data.clear();
        foreach (QVariant v, aData.toVariantList())
        {
            data.append((char) v.toInt());
        }
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
}

void LuaBridgeDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
}

void LuaBridgeDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
}

void LuaBridgeDevice::putFile(QByteArray name, unsigned int size)
{
}

void LuaBridgeDevice::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
}



void LuaBridgeDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}
