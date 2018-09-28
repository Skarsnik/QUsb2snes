#include "luabridge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

#include "rommapping/rommapping.h"

Q_LOGGING_CATEGORY(log_luaBridge, "LUABridge")
#define sDebug() qCDebug(log_luaBridge)

LuaBridge::LuaBridge()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    timer.setInterval(3);
    timer.setSingleShot(true);
    m_state = CLOSED;
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    tcpServer->listen(QHostAddress("127.0.0.1"), 65398);
    sDebug() << "LUA bridge created";
    client = nullptr;
}

static unsigned int usb2snes_addr_to_snes(unsigned int addr)
{
    if (addr >= 0xF50000 && addr < 0xF70000)
        return addr - 0xF50000 + 0x7E0000;
    if (addr >= 0xE00000)
        return rommapping_sram_pc_to_snes(addr - 0xE00000, LoROM, false);
    return rommapping_pc_to_snes(addr, LoROM, false);
}

void LuaBridge::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    QByteArray toWrite = "Read|" + QByteArray::number(usb2snes_addr_to_snes(addr)) + "|" + QByteArray::number(size) + "\n";
    sDebug() << ">>" << toWrite;
    sDebug() << "Writen" << client->write(toWrite) << "Bytes";
    m_state = BUSY;
}

void LuaBridge::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    m_state = BUSY;
    putAddr = addr;
    putSize = size;
}

void LuaBridge::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}

void LuaBridge::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    putAddrCommand(space, addr, size);
}


void LuaBridge::infoCommand()
{
    m_state = BUSY;
    timer.start();
}

void LuaBridge::writeData(QByteArray data)
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
        client->write(toSend);
        emit commandFinished();
        receivedSize = 0;
        received.clear();
        m_state = READY;
    }
}

QString LuaBridge::name() const
{
    return "EMU SNES9X";
}

bool LuaBridge::hasFileCommands()
{
    return false;
}

bool LuaBridge::hasControlCommands()
{
    return false;
}

USB2SnesInfo LuaBridge::parseInfo(const QByteArray &data)
{
    USB2SnesInfo info;
    info.romPlaying = "No Info";
    info.version = "1.0.0";
    return info;
}

QList<ADevice::FileInfos> LuaBridge::parseLSCommand(QByteArray &dataI)
{
    return QList<ADevice::FileInfos>();
}

bool LuaBridge::canAttach()
{
    if (client != nullptr)
        return true;
    m_attachError = "No LUAbridge lua script connected";
    return false;
}

bool LuaBridge::open()
{
    m_state = READY;
    return true;
}

void LuaBridge::close()
{
    tcpServer->close();
}

void LuaBridge::onNewConnection()
{
    sDebug() << "Client connected";
    client = tcpServer->nextPendingConnection();
    connect(client, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
    connect(client, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
}

void LuaBridge::onServerError()
{
    m_state = CLOSED;
    emit closed();
}

void LuaBridge::onClientReadyRead()
{
    static      QByteArray  dataRead = QByteArray();

    QByteArray  data = client->readAll();
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

void LuaBridge::onClientDisconnected()
{
    sDebug() << "Client disconnected";
    client->deleteLater();
    m_state = CLOSED;
    client = nullptr;
    emit closed();
}

void LuaBridge::onTimerOut()
{
    m_state = READY;
    sDebug() << "Fake command finished";
    emit commandFinished();
}



void LuaBridge::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
}

void LuaBridge::fileCommand(SD2Snes::opcode op, QByteArray args)
{
}

void LuaBridge::controlCommand(SD2Snes::opcode op, QByteArray args)
{
}

void LuaBridge::putFile(QByteArray name, unsigned int size)
{
}

void LuaBridge::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2)
{
}

