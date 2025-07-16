#include "remoteusb2sneswdevice.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_remoteusb2snesdevice, "Remote Device")
#define sDebug() qCDebug(log_remoteusb2snesdevice)
#define sInfo() qCInfo(log_remoteusb2snesdevice)


RemoteUsb2snesWDevice::RemoteUsb2snesWDevice(QObject *parent)
    : ADevice{parent}
{
}

void RemoteUsb2snesWDevice::createWebsocket(QUrl url)
{
    websocket->open(url);
    connect(websocket, &QWebSocket::textMessageReceived, this, &RemoteUsb2snesWDevice::textMessageReceived);
    connect(websocket, &QWebSocket::binaryMessageReceived, this, &RemoteUsb2snesWDevice::binaryMessageReceived);
}

void RemoteUsb2snesWDevice::send(QString message)
{
    sDebug() << "Sending text request to remote" << message;
    websocket->sendTextMessage(message);
}

void RemoteUsb2snesWDevice::send(QByteArray data)
{
    sDebug() << "Sending binary data to remote" << data.size();
    websocket->sendBinaryMessage(data);
}



//USELESS
void RemoteUsb2snesWDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{

}

void RemoteUsb2snesWDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{

}

void RemoteUsb2snesWDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{

}

void RemoteUsb2snesWDevice::putFile(QByteArray name, unsigned int size)
{

}

void RemoteUsb2snesWDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{

}

void RemoteUsb2snesWDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{

}

void RemoteUsb2snesWDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{

}

void RemoteUsb2snesWDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{

}

void RemoteUsb2snesWDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{

}

void RemoteUsb2snesWDevice::infoCommand()
{

}

void RemoteUsb2snesWDevice::writeData(QByteArray data)
{

}

QString RemoteUsb2snesWDevice::name() const
{
    return "Usb2Snes remote";
}

bool RemoteUsb2snesWDevice::hasFileCommands()
{
    return false;
}

bool RemoteUsb2snesWDevice::hasControlCommands()
{
    return false;
}

USB2SnesInfo RemoteUsb2snesWDevice::parseInfo(const QByteArray &data)
{
    return USB2SnesInfo();
}

QList<ADevice::FileInfos> RemoteUsb2snesWDevice::parseLSCommand(QByteArray &dataI)
{
    return QList<ADevice::FileInfos>();
}

bool RemoteUsb2snesWDevice::open()
{
    return false;
}

void RemoteUsb2snesWDevice::close()
{

}
