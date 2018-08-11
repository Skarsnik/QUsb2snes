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
        sDebug() << "WebSocket server started : listenning on port 8080";
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
    wi.name = "Websocket " + QString::number((size_t)ws, 16);
    wi.attached = false;
    wi.attachedTo = NULL;
    wi.commandState = NOCOMMAND;
    wi.byteReceived = 0;
    wi.pendingAttach = false;
    wi.recvData.clear();
    wsInfos[newSocket] = wi;
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
    sDebug() << "Adding device" << device->name();
    devices.append(device);
    devicesInfos[device] = DeviceInfos();
    connect(device, SIGNAL(commandFinished()), this, SLOT(onDeviceCommandFinished()));
    connect(device, SIGNAL(protocolError()), this, SLOT(onDeviceProtocolError()));
    connect(device, SIGNAL(closed()), this, SLOT(onDeviceClosed()));
    sDebug() << "Added device : " << device->name();
}

void WSServer::removeDevice(ADevice *device)
{
    setError(DeviceError, "Manualy removed the device (probably from the UI");
    cleanUpDevice(device);
    devices.removeAt(devices.indexOf(device));
    devicesInfos.remove(device);

    // TODO remove and disconnect WS
}

void    WSServer::cleanUpDevice(ADevice* device)
{
    DeviceInfos& devInfo = devicesInfos[device];
    QMapIterator<QWebSocket*, WSInfos> it(wsInfos);
    QList<QWebSocket*> toDiscard;
    while (it.hasNext())
    {
        if (it.value().attachedTo == device)
            toDiscard.append(it.key());
    }
    foreach (QWebSocket* ws, toDiscard)
        clientError(ws);
    if (devInfo.currentWS != NULL)
        devInfo.currentWS = NULL;
}

QMap<QString, QStringList> WSServer::getDevicesInfo()
{
    QMap<QString, QStringList> toret;
    QListIterator<ADevice*> it(devices);
    QStringList canAttachDev = getDevicesList();

    while (it.hasNext())
    {
        ADevice *dev = it.next();
        if (!canAttachDev.contains(dev->name()))
            continue;
        toret[dev->name()] = QStringList();
        QMapIterator<QWebSocket*, WSInfos> wsIit(wsInfos);
        while (wsIit.hasNext())
        {
            auto p = wsIit.next();
            if (p.value().attachedTo == dev)
            {
                toret[dev->name()] << p.value().name;
            }
        }
    }
    foreach (QString sdev, canAttachDev) {
        if (!toret.contains(sdev))
            toret[sdev] = QStringList();
    }
    return toret;
}


void WSServer::onTextMessageReceived(QString message)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    const WSInfos &wsInfo = wsInfos.value(ws);
    sDebug() << wsInfo.name << "received " << message;

    MRequest* req = requestFromJSON(message);
    sDebug() << "Request is " << req->opcode;
    if ((intptr_t)req->owner == 42)
        goto LError;
    req->owner = ws;
    if (wsInfo.attached == false && !isValidUnAttached(req->opcode))
    {
        if (wsInfo.pendingAttach)
        {
            sDebug() << wsInfo.attachedTo->name() << "Adding request in queue " << *req << "(" << pendingRequests[wsInfo.attachedTo].size() << ")";
            addToPendingRequest(wsInfo.attachedTo, req);
            return ;
        } else {
            setError(ErrorType::ProtocolError, "Invalid command while unattached");
            goto LError;
        }
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
        sDebug() << "Adding a Put command in queue, adding size" << wsInfos[req->owner].pendingPutSizes.last();
    }
}

void WSServer::onBinaryMessageReceived(QByteArray data)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    WSInfos& infos = wsInfos[ws];
    infos.recvData.append(data);
    infos.byteReceived += data.size();
    ADevice* dev = wsInfos.value(ws).attachedTo;
    sDebug() << infos.name << "Received binary data" << data.size();
    if (infos.commandState != ClientCommandState::WAITINGBDATAREPLY)
    {
        setError(ErrorType::ProtocolError, "Sending binary data when nothing waiting for it");
        clientError(ws);
    } else {
        QList<unsigned int>& pend = infos.pendingPutSizes;
        if (pend.isEmpty()) {
            infos.byteReceived = 0;
            infos.recvData.clear();
            dev->writeData(data);
        } else {
            sDebug() << "Data for pending request" << infos.byteReceived << pend.first();
            if (pend.first() == infos.byteReceived)
            {
                sDebug() << infos.name << "Putting data in queue";
                infos.pendingPutDatas.append(infos.recvData);
                pend.removeAt(0);
                infos.byteReceived = 0;
                infos.recvData.clear();
            }
        }

    }
}

void WSServer::onClientDisconnected()
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sDebug() << "Websocket disconnected" << wsInfos.value(ws).name;
    cleanUpSocket(ws);
}

void WSServer::onClientError(QAbstractSocket::SocketError)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sDebug() << "Client error : " << wsInfos.value(ws).name << ws->errorString();
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
    ADevice*  device = qobject_cast<ADevice*>(sender());
    setError(ProtocolError, "Error in device protocol");
    cleanUpDevice(device);
}

void WSServer::onDeviceClosed()
{
    cleanUpDevice(qobject_cast<ADevice*>(sender()));
}

void WSServer::onDeviceGetDataReceived(QByteArray data)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos.value(device).currentWS == NULL)
    {
        sDebug() << "NOOP Sending get data to nothing" << device->name();
        return ;
    }
    sDebug() << "Sending " << data.size() << "to" << wsInfos.value(devicesInfos[device].currentWS).name;
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
    WSInfos wInfo = wsInfos.value(ws);
    sDebug() << "Cleaning up " << wInfo.name;
    if (wInfo.attached)
    {
        ADevice*    dev = wInfo.attachedTo;
        MRequest*   req = currentRequests[dev];
        if (devicesInfos.value(dev).currentWS == ws)
            devicesInfos[dev].currentWS = NULL;
        if (req != NULL && req->owner == ws)
        {
            req->owner = NULL;
            req->state = RequestState::CANCELLED;
        }
        // Removing pending request that are tied to this ws
        QMutableListIterator<MRequest*>    it(pendingRequests[dev]);
        while(it.hasNext())
        {
            MRequest* mReq = it.next();
            if (mReq->owner == ws)
            {
                it.remove();
                delete mReq;
            }
        }
    }
    wsInfos.remove(ws);
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
    if (ws == NULL)
    {
        sDebug() << "NOOP: Sending reply to a non existing client";
        return ;
    }
    QJsonObject jObj;
    QJsonArray ja;

    foreach(QString s, args)
    {
        ja.append(QJsonValue(s));
    }
    jObj["Results"] = ja;
    sDebug() << wsInfos.value(ws).name << ">>" << QJsonDocument(jObj).toJson();
    ws->sendTextMessage(QJsonDocument(jObj).toJson());
}

void    WSServer::sendReply(QWebSocket *ws, QString args)
{
    sendReply(ws, QStringList() << args);
}

QDebug operator<<(QDebug debug, const WSServer::MRequest &req)
{
    debug << req.id << "Created at" << req.timeCreated << "-" << req.opcode << req.space << req.flags << req.arguments << req.state;
    return debug;
}
