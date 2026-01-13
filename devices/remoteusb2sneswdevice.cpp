#include "remoteusb2sneswdevice.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_remoteusb2snesdevice, "Remote Device")
#define sDebug() qCDebug(log_remoteusb2snesdevice)
#define sInfo() qCInfo(log_remoteusb2snesdevice)


RemoteUsb2snesWDevice::RemoteUsb2snesWDevice(QString remoteName, QObject *parent)
    : ADevice{parent}
{
    sDebug() << "Creating remote device to " << remoteName;
    remoteDeviceName = remoteName;
}

void RemoteUsb2snesWDevice::createWebsocket(QUrl url)
{
    websocket = new QWebSocket();
    websocket->open(url);
    // We can't connect yet to the remote server, attach need to be first
    connect(websocket, &QWebSocket::textMessageReceived, this, [=] (const QString& message)
    {
        if (attached == false)
        {
            Message m;
            m.text = message;
            queue.enqueue(m);
            sDebug() << "Queueing text message";
        }
        sDebug() << "Received message from remote" << message;
    });
    connect(websocket, &QWebSocket::binaryMessageReceived, this, [=] (const QByteArray& message)
    {
        if (attached == false)
        {
            Message m;
            m.datas = message;
            queue.enqueue(m);
            sDebug() << "Queueing data message";
        }
        sDebug() << "Received binary message from remote, size : " << message.size();
    });
    connect(websocket, &QWebSocket::connected, this, [=]
    {
        sDebug() << "Connected to remote " << url;
        attach();
    });
    connect(websocket, &QWebSocket::disconnected, this, [=]
    {
        sDebug() << "Disconnected";
        emit closed();
    });
}

void RemoteUsb2snesWDevice::sendText(const QString& message)
{
    if (attached == false)
    {
        Message m;
        m.text = message;
        queue.enqueue(m);
        sDebug() << "Queueing text message" << message;
        return ;
    }
    sDebug() << "Sending text request to remote" << message;
    websocket->sendTextMessage(message);
}

void RemoteUsb2snesWDevice::sendBinary(const QByteArray& data)
{
    if (attached == false)
    {
        Message m;
        m.datas = data;
        queue.enqueue(m);
        sDebug() << "Queueing data message" << data;
        return ;
    }
    sDebug() << "Sending binary data to remote" << data.size();
    websocket->sendBinaryMessage(data);
}

void RemoteUsb2snesWDevice::setClientName(const QString &name)
{
    clientName = name;
}

void RemoteUsb2snesWDevice::attach()
{
    QJsonArray      jOp;
    QJsonObject     jObj;

    sDebug() << "Attaching to remote";
    jObj["Opcode"] = "Attach";
    jOp.append(remoteDeviceName);
    jObj["Space"] = "SNES";
    jObj["Operands"] = jOp;
    sDebug() << ">>" << QJsonDocument(jObj).toJson();
    websocket->sendTextMessage(QJsonDocument(jObj).toJson());
    attached = true;
    if (clientName.isEmpty() == false)
    {
        jObj["Opcode"] = "Name";
        jOp[0] = "Remote " + clientName;
        jObj["Operands"] = jOp;
        sDebug() << ">>" << QJsonDocument(jObj).toJson();
        websocket->sendTextMessage(QJsonDocument(jObj).toJson());
    }
    while (queue.empty() == false)
    {
        qDebug() << "Sending queued messages " << queue.size();
        Message m = queue.dequeue();
        if (m.text.isEmpty() == false)
        {
            websocket->sendTextMessage(m.text);
        } else {
            websocket->sendBinaryMessage(m.datas);
        }
    }
    connect(websocket, &QWebSocket::textMessageReceived, this, &RemoteUsb2snesWDevice::textMessageReceived);
    connect(websocket, &QWebSocket::binaryMessageReceived, this, &RemoteUsb2snesWDevice::binaryMessageReceived);
}


QString RemoteUsb2snesWDevice::name() const
{
    return "Usb2Snes remote";
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
