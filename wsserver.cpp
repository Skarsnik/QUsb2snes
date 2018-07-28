#include "wsserver.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_wsserver, "WSServer")
#define sDebug() qCDebug(log_wsserver)

quint64 WSServer::MRequest::gId = 0;

WSServer::WSServer(QObject *parent) : QObject(parent)
{
    wsServer = new QWebSocketServer(QStringLiteral("USB2SNES Server"), QWebSocketServer::NonSecureMode, this);
    const QMetaObject &mo = USB2SnesWS::staticMetaObject;
    const QMetaObject &mo2 = SD2Snes::staticMetaObject;
    int i = mo.indexOfEnumerator("opcode");
    cmdMetaEnum = mo.enumerator(i);
    i = mo2.indexOfEnumerator("space");
    spaceMetaEnum = mo2.enumerator(i);
}

bool WSServer::start()
{
    if (wsServer->listen(QHostAddress::Any, 8080))
    {
        sDebug() << "Webserver started : listenning on port 8080";
        connect(wsServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
        connect(wsServer, SIGNAL(closed()), this, SLOT(onWSClosed()));
        connect(wsServer, SIGNAL(serverError(QWebSocketProtocol::CloseCode)), this, SLOT(onWSError(QWebSocketProtocol::CloseCode)));
        return true;
    }
    return false;
}

void WSServer::onNewConnection()
{
    QWebSocket* newSocket = wsServer->nextPendingConnection();

    connect(newSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onTextMessageReceived(QString)));
    connect(newSocket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(onBinaryMessageReceived(QByteArray)));
    connect(newSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    connect(newSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onClientError(QAbstractSocket::SocketError)));

    WSInfos wi;
    wi.attached = false;
    wi.attachedTo = NULL;
    wsInfos[newSocket] = wi;

    wsNames[newSocket] = "Websocket" + QString::number((size_t)ws, 16);
    unassignedWS << newSocket;
}

void WSServer::onWSError(QWebSocketProtocol::CloseCode code)
{
    sDebug() << "Websocket error";
}

void WSServer::onWSClosed()
{
    qDebug() << "Websocket closed";
}

void WSServer::addDevice(ADevice *device)
{
    devices.append(device);
    devicesInfos[device] = DeviceInfos();
    connect(device, SIGNAL(commandFinished()), this, SLOT(onDeviceCommandFinished()));
    connect(device, SIGNAL(protocolError()), this, SLOT(onDeviceProtocolError()));
    connect(device, SIGNAL(closed()), this, SLOT(onDeviceClosed()));
    sDebug() << "Added device : " << device->name();
}

void WSServer::removeDevice(ADevice *device)
{
    devices.removeAt(devices.indexOf(device));
    devicesInfos.remove(device);
    // TODO remove and disconnect WS
}

QMap<QString, QStringList> WSServer::getDevicesInfo()
{
    QMap<QString, QStringList> toret;
    QListIterator<ADevice*> it(devices);
    while (it.hasNext())
    {
        ADevice *dev = it.next();
        toret[dev->name()] = QStringList();
        QMapIterator<QWebSocket*, WSInfos> wsIit(wsInfos);
        while (wsIit.hasNext())
        {
            auto p = wsIit.next();
            if (p.value().attachedTo == dev)
            {
                toret[dev->name()] << wsNames.value(p.key());
            }
        }
    }
    return toret;
}


void WSServer::onTextMessageReceived(QString message)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    const WSInfos &wsInfo = wsInfos.value(ws);
    sDebug() << wsNames.value(ws) << "received " << message;

    MRequest* req = requestFromJSON(message);
    sDebug() << "Request is " << req->opcode;
    if ((intptr_t)req->owner == 42)
        goto LError;
    req->owner = ws;
    if (wsInfo.attached == false && !isValidUnAttached(req->opcode))
    {
        setError(ErrorType::ProtocolError, "Invalid command while unattached");
        goto LError;
    }
    if (wsInfo.attached && !isValidUnAttached(req->opcode))
    {
        if (wsInfo.attachedTo->state() == ADevice::READY)
        {
            ADevice* lowCo = wsInfo.attachedTo;
            if ((isControlCommand(req->opcode) && !lowCo->hasControlCommands()) ||
                 isFileCommand(req->opcode) && !lowCo->hasFileCommands())
            {
                setError(ErrorType::DeviceError, QString("The device does not support the command %1").arg(cmdMetaEnum.valueToKey(static_cast<int>(req->opcode))));
                goto LError;
            }
            currentRequests[lowCo] = req;
            devicesInfos[lowCo].currentCommand = req->opcode;
            devicesInfos[lowCo].currentWS = ws;
            executeRequest(req);
        } else { // add to queue
            sDebug() << wsInfo.attachedTo->name() << "Adding request in queue " << *req << "(" << pendingRequests[wsInfo.attachedTo].size() << ")";
            addToPendingRequest(wsInfo.attachedTo, req);
        }

    }
    if (isValidUnAttached(req->opcode))
        executeRequest(req);
    return ;
LError:
    clientError(ws);
}

void    WSServer::addToPendingRequest(ADevice* device, MRequest *req)
{
    pendingRequests[device].append(req);
    if (req->opcode == USB2SnesWS::PutAddress)
    {
        bool ok;
        wsInfos[req->owner].pendingPutSizes.append(req->arguments.at(1).toInt(&ok, 16));
    }
}

void WSServer::onBinaryMessageReceived(QByteArray data)
{
    // This need to be per ws
    static unsigned int byteReceived = 0;
    static QByteArray   recvData = QByteArray();

    recvData.append(data);
    byteReceived += data.size();
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    WSInfos& infos = wsInfos[ws];
    ADevice* dev = wsInfos.value(ws).attachedTo;
    sDebug() << wsNames.value(ws) << "Received binary data" << data.size();
    if (infos.commandState != ClientCommandState::WAITINGBDATAREPLY)
    {
        setError(ErrorType::ProtocolError, "Sending binary data when nothing waiting for it");
        clientError(ws);
    } else {
        QList<unsigned int>& pend = infos.pendingPutSizes;
        if (pend.isEmpty()) {
            dev->writeData(data);
            byteReceived = 0;
            recvData.clear();
        }
        else
        {
            if (pend.first() == byteReceived)
            {
                sDebug() << wsNames.value(ws) << "Putting data in queue";
                infos.pendingPutDatas.append(recvData);
                pend.removeAt(0);
                byteReceived = 0;
                recvData.clear();
            }
        }

    }
}

void WSServer::onClientDisconnected()
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sDebug() << "Websocket disconnected" << wsNames.value(ws);
    cleanUpSocket(ws);
}

void WSServer::onClientError(QAbstractSocket::SocketError)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sDebug() << "Client error : " << wsNames.value(ws) << ws->errorString();
}

void WSServer::onDeviceCommandFinished()
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos[device].currentWS != NULL)
    {
        processDeviceCommandFinished(device);
        processCommandQueue(device);
    }
    else
        sDebug() << "Received finished command while no socket to receive it";
}

void WSServer::onDeviceProtocolError()
{

}

void WSServer::onDeviceClosed()
{

}

void WSServer::onDeviceGetDataReceived(QByteArray data)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    sDebug() << "Sending " << data.size() << "to" << wsNames.value(devicesInfos[device].currentWS);
    devicesInfos[device].currentWS->sendBinaryMessage(data);
}

void WSServer::onDeviceSizeGet(unsigned int size)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    sendReply(devicesInfos[device].currentWS, QString::number(size, 16));
}

void        WSServer::processCommandQueue(ADevice* device)
{
    QList<MRequest*>&    cmdQueue = pendingRequests[device];
    if (!cmdQueue.isEmpty())
    {
        sDebug() << cmdQueue.size() << " requests in queue, processing the first";
        MRequest* req = cmdQueue.takeFirst();
        currentRequests[device] = req;
        devicesInfos[device].currentCommand = req->opcode;
        devicesInfos[device].currentWS = req->owner;
        req->wasPending = true;
        executeRequest(req);
    }
}

void WSServer::setError(const WSServer::ErrorType type, const QString reason)
{
    m_errorType = type;
    m_errorString = reason;
}

WSServer::MRequest* WSServer::requestFromJSON(const QString &str)
{
    MRequest    *req = new MRequest();
    req->state = RequestState::NEW;
    req->owner = NULL;
    QJsonDocument   jdoc = QJsonDocument::fromJson(str.toLatin1());
    QJsonObject job = jdoc.object();
    QString opcode = job["Opcode"].toString();
    if (cmdMetaEnum.keyToValue(qPrintable(opcode)) == -1)
    {
        req->owner = (QWebSocket*)(42);
        setError(ErrorType::ProtocolError, "Invalid OPcode send" + opcode);
        return req;
    }
    if (job.contains("Space"))
    {
        QString space = job["Space"].toString();

        if (spaceMetaEnum.keyToValue(qPrintable(space)) == -1)
        {
            req->owner = (QWebSocket*)(42);
            setError(ErrorType::ProtocolError, "Invalid Space send" + space);
            return req;
        }
        req->space = (SD2Snes::space) spaceMetaEnum.keyToValue(qPrintable(space));
    }
    req->opcode = (USB2SnesWS::opcode) cmdMetaEnum.keyToValue(qPrintable(opcode));
    if (job.contains("Operands"))
    {
        QJsonArray   jarray = job["Operands"].toArray();
        foreach(QVariant entry, jarray.toVariantList())
        {
            req->arguments << entry.toString();
        }
    }
    if (job.contains("Flags"))
    {
        QJsonArray   jarray = job["Flags"].toArray();
        foreach(QVariant entry, jarray.toVariantList())
        {
            req->flags << entry.toString();
        }
    }
    req->timeCreated = QTime::currentTime();
    return req;
}

void WSServer::clientError(QWebSocket *ws)
{
    sDebug() << "Error with a ws client" << m_errorType << m_errorString;
    ws->close();
    cleanUpSocket(ws);
    emit error();
}

void WSServer::cleanUpSocket(QWebSocket *ws)
{
    sDebug() << "Cleaning up " << wsNames[ws];
    wsInfos.remove(ws);
    wsNames.remove(ws);
    unassignedWS.removeAll(ws);
    QMutableMapIterator<ADevice*, DeviceInfos> i(devicesInfos);
    while (i.hasNext())
    {
        i.next();
        if (i.value().currentWS == ws)
        {
            i.value().currentWS = NULL;
            break;
        }
    }
    ws->deleteLater();
}

bool WSServer::isValidUnAttached(const USB2SnesWS::opcode opcode)
{
    if (opcode == USB2SnesWS::Attach ||
        opcode == USB2SnesWS::AppVersion ||
        opcode == USB2SnesWS::Name ||
        opcode == USB2SnesWS::DeviceList)
        return true;
    return false;
}

void        WSServer::sendReply(QWebSocket* ws, const QStringList& args)
{
    QJsonObject jObj;
    QJsonArray ja;

    foreach(QString s, args)
    {
        ja.append(QJsonValue(s));
    }
    jObj["Results"] = ja;
    sDebug() << wsNames[ws] << ">>" << QJsonDocument(jObj).toJson();
    ws->sendTextMessage(QJsonDocument(jObj).toJson());
}

void    WSServer::sendReply(QWebSocket *ws, QString args)
{
    sendReply(ws, QStringList() << args);
}

QDebug operator<<(QDebug debug, const WSServer::MRequest &req)
{
    debug << req.id << "Created at" << req.timeCreated << "-" << req.opcode << req.space << req.flags << req.arguments;
    return debug;
}
