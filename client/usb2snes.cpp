/*
    This file is part of the SaveState2snes software
    Copyright (C) 2017  Sylvain "Skarsnik" Colinet <scolinet@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "usb2snes.h"
#include <QUrl>
#include <QDebug>

Q_LOGGING_CATEGORY(log_Usb2snes, "USB2SNES")
#define sDebug() qCDebug(log_Usb2snes)

USB2snes::USB2snes(bool autoAttach = true) : QObject()
{
    m_state = None;
    m_istate = INone;

    QObject::connect(&m_webSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onWebSocketTextReceived(QString)));
    QObject::connect(&m_webSocket, SIGNAL(connected()), this, SLOT(onWebSocketConnected()));
    QObject::connect(&m_webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWebSocketError(QAbstractSocket::SocketError)));
    QObject::connect(&m_webSocket, SIGNAL(disconnected()), this, SLOT(onWebSocketDisconnected()));
    QObject::connect(&m_webSocket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(onWebSocketBinaryReceived(QByteArray)));
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerTick()));
    requestedBinaryReadSize = 0;
    m_autoAttach = autoAttach;
}

void    USB2snes::usePort(QString port)
{
    m_port = port;
}

QString USB2snes::port()
{
    return m_port;
}

QString USB2snes::getRomName()
{
    //Need to gather data then check if this is consistnt for lorom, otherwise it's hirom
    QByteArray title = getAddress(0x7FC0, 21);
    QByteArray romMakeUp = getAddress(0x7FD5, 1);
    //QByteArray tmp = getAddress(0xC0FFD5, 2);

    if (romMakeUp[0] & 0x1)
    {
        title = getAddress(0xFFC0, 21);
    }
    return QString(title);
}

void USB2snes::connect()
{
    if (m_state == None)
        m_webSocket.open(QUrl(USB2SNESURL));
}

void USB2snes::close()
{
    if (m_state != None)
       m_webSocket.close();
}

void USB2snes::setAppName(QString name)
{
    sendRequest("Name", QStringList() << name);
}

void USB2snes::attach(QString deviceName)
{
    sendRequest("Attach", QStringList() << deviceName);
    changeState(Ready);
}

void USB2snes::onWebSocketConnected()
{
    sDebug() << "Websocket connected";
    changeState(Connected);
    m_istate = IConnected;
    if (m_autoAttach)
    {
        m_istate = DeviceListRequested;
        sendRequest("DeviceList");
    }
}

void USB2snes::onWebSocketDisconnected()
{
    sDebug() << "Websocket disconnected";
    changeState(None);
    m_istate = INone;
    lastBinaryMessage = "";
    lastTextMessage = "";
    emit disconnected();
}

QStringList USB2snes::getJsonResults(QString json)
{
    QStringList toret;
    QJsonDocument   jdoc = QJsonDocument::fromJson(json.toLatin1());
    if (!jdoc.object()["Results"].toArray().isEmpty())
    {
        QJsonArray jarray = jdoc.object()["Results"].toArray();
        foreach(QVariant entry, jarray.toVariantList())
        {
            toret << entry.toString();
        }
    }
    return toret;
}

void USB2snes::onWebSocketTextReceived(QString message)
{
    sDebug() << "<<T" << message;
    lastTextMessage = message;
    switch (m_istate)
    {
    case DeviceListRequested:
    {
        QStringList results = getJsonResults(message);
        m_deviceList = results;
        if (m_autoAttach)
        {
            if (!results.isEmpty())
            {
                timer.stop();
                m_port = results.at(0);
                sendRequest("Attach", QStringList() << m_port);
                m_istate = AttachSent;
                timer.start(200);
            } else {
                timer.start(1000);
            }
        }
        break;
    }
    case FirmwareVersionRequested:
    {
        QStringList results = getJsonResults(message);
        if (!results.isEmpty())
        {
            m_firmwareString = results.at(0);
            if (m_firmwareString.right(3) == "gsu")
                m_firmwareVersion = QVersionNumber(7);
            else
                m_firmwareVersion = QVersionNumber(m_firmwareString.right(1).toInt());
            m_istate = ServerVersionRequested;
            sendRequest("AppVersion");
        }
        break;
    }
    case ServerVersionRequested:
    {
        QStringList results = getJsonResults(message);
        if (!results.isEmpty())
        {
            m_serverVersion = QVersionNumber::fromString(results.at(0));
            m_istate = IReady;
            changeState(Ready);
        }
        break;
    }
    default:
        break;
    }
    emit textMessageReceived();
}

void USB2snes::onWebSocketBinaryReceived(QByteArray message)
{
    static QByteArray buffer;
    if (message.size() < 100)
      sDebug() << "<<B" << message.toHex('-') << message;
    else
      sDebug() << "<<B" << "Received " << message.size() << " byte of data";
    buffer.append(message);
    if ((unsigned int) buffer.size() == requestedBinaryReadSize)
    {
        lastBinaryMessage = buffer;
        emit binaryMessageReceived();
        buffer.clear();
    }
}

void USB2snes::onWebSocketError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    sDebug() << "Error " << m_webSocket.errorString();
}

void USB2snes::onTimerTick()
{
    if (m_istate == AttachSent)
    {
        sendRequest("Info");
        m_istate = FirmwareVersionRequested;
        timer.stop();
    }
    if (m_istate == DeviceListRequested)
    {
        sendRequest("DeviceList");
    }
}


void USB2snes::sendRequest(QString opCode, QStringList operands, Space space, QStringList flags)
{
    Q_UNUSED(flags)
    QJsonArray      jOp;
    QJsonObject     jObj;

    jObj["Opcode"] = opCode;
    if (space == SNES)
        jObj["Space"] = "SNES";
    if (space == CMD)
        jObj["Space"] = "CMD";
    foreach(QString sops, operands)
        jOp.append(QJsonValue(sops));
    if (!operands.isEmpty())
        jObj["Operands"] = jOp;
    sDebug() << ">>" << QJsonDocument(jObj).toJson();
    m_webSocket.sendTextMessage(QJsonDocument(jObj).toJson());
}

void USB2snes::changeState(USB2snes::State s)
{
    m_state = s;
    sDebug() << "State changed to " << s;
    emit stateChanged();
}

QByteArray USB2snes::getAddress(unsigned int addr, unsigned int size, Space space)
{
    m_istate = IBusy;
    sendRequest("GetAddress", QStringList() << QString::number(addr, 16) << QString::number(size, 16), space);
    requestedBinaryReadSize = size;
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(binaryMessageReceived()), &loop, SLOT(quit()));
    QObject::connect(this, SIGNAL(disconnected()), &loop, SLOT(quit()));
    loop.exec();
    requestedBinaryReadSize = 0;
    sDebug() << "Getting data,  size : " << lastBinaryMessage.size() << "- MD5 : " << QCryptographicHash::hash(lastBinaryMessage, QCryptographicHash::Md5).toHex();
    m_istate = IReady;
    return lastBinaryMessage;
}

void USB2snes::setAddress(unsigned int addr, QByteArray data, Space space)
{
    m_istate = IBusy;
    sendRequest("PutAddress", QStringList() << QString::number(addr, 16) << QString::number(data.size(), 16), space);
    //Dumb shit for bad win7 C# websocket api
    sDebug() << "Sending data,  size : " << data.size() << "- MD5 : " << QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    if (data.size() <= 1024)
      m_webSocket.sendBinaryMessage(data);
    else
    {
        while (data.size() != 0)
        {
            m_webSocket.sendBinaryMessage(data.left(1024));
            data.remove(0, 1024);
        }
    }
    m_istate = IReady;
    sDebug() << "Done sending data for setAddress " + QString::number(addr, 16);
}

void USB2snes::sendFile(QString path, QByteArray data)
{
    sendRequest("PutFile", QStringList() << path << QString::number(data.size(), 16));
    changeState(SendingFile);
    fileDataToSend = data;
    m_istate = IBusy;
    if (data.size() <= 1024)
      m_webSocket.sendBinaryMessage(data);
    else
    {
        int foo = 0;
        while (data.size() != 0)
        {
            m_webSocket.sendBinaryMessage(data.left(1024));
            data.remove(0, 1024);
            foo += 1024;
            emit fileSendProgress(foo);
        }
    }
    emit fileSent();
    m_istate = IReady;
    changeState(Ready);
}

void USB2snes::getFile(QString path)
{
    sendRequest("GetFile", QStringList() << path);
    changeState(SendingFile);
}

void USB2snes::renameFile(QString oldPath, QString newPath)
{
    sendRequest("Rename", QStringList() << oldPath << newPath);
}

void USB2snes::deleteFile(QString fileName)
{
    sendRequest("Remove", QStringList() << fileName);
}

void USB2snes::boot(QString path)
{
    sendRequest("Boot", QStringList() << path);
}


bool USB2snes::patchROM(QString patch)
{
    QFile fPatch(patch);
    if (fPatch.open(QIODevice::ReadOnly))
    {
        unsigned int size = fPatch.size();
        sendRequest("PutIPS", QStringList() << "hook" << QString::number(size, 16));
        QByteArray data = fPatch.readAll();
        m_webSocket.sendBinaryMessage(data);
        return true;
    }
    return false;
}


USB2snes::State USB2snes::state()
{
    return m_state;
}

QStringList USB2snes::infos()
{
    if (m_istate != IReady)
        return QStringList();
    sendRequest("Info");
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(textMessageReceived()), &loop, SLOT(quit()));
    loop.exec();
    return getJsonResults(lastTextMessage);
}

int USB2snes::fileDataSize() const
{
    return fileDataToSend.size();
}

QList<USB2snes::FileInfo> USB2snes::ls(QString path)
{
    QList<FileInfo> toret;
    sendRequest("List", QStringList() << path);
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(textMessageReceived()), &loop, SLOT(quit()));
    loop.exec();
    QStringList infos = getJsonResults(lastTextMessage);
    for (int i = 0; i < infos.size(); i += 2)
    {
        FileInfo fi;
        fi.dir = infos.at(i) == "0";
        fi.name = infos.at(i + 1);
        toret << fi;
    }
    return toret;
}

QString USB2snes::firmwareString()
{
    return m_firmwareString;
}

QVersionNumber USB2snes::firmwareVersion()
{
    return m_firmwareVersion;
}


QStringList USB2snes::deviceList()
{
    sendRequest("DeviceList");
    QEventLoop  loop;
    QObject::connect(this, SIGNAL(textMessageReceived()), &loop, SLOT(quit()));
    loop.exec();
    return getJsonResults(lastTextMessage);
}

QVersionNumber USB2snes::serverVersion()
{
    return m_serverVersion;
}
