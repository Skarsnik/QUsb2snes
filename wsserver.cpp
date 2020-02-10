#include "wsserver.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSettings>

Q_LOGGING_CATEGORY(log_wsserver, "WSServer")
#define sDebug() qCDebug(log_wsserver)
#define sInfo() qCInfo(log_wsserver)

extern QSettings*          globalSettings;

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
    i = mo2.indexOfEnumerator("server_flags");
    flagsMetaEnum = mo2.enumerator(i);
    trustedOrigin.append("http://localhost");
    trustedOrigin.append("");
}

bool WSServer::start(QHostAddress lAddress, quint16 port)
{
    if (wsServer->listen(lAddress, port))
    {
        sInfo() << "WebSocket server started : listenning " << lAddress << "port : " << port;
        connect(wsServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
        connect(wsServer, SIGNAL(closed()), this, SLOT(onWSClosed()));
        connect(wsServer, SIGNAL(serverError(QWebSocketProtocol::CloseCode)), this, SLOT(onWSError(QWebSocketProtocol::CloseCode)));
        return true;
    }
    emit listenFailed(wsServer->errorString());
    return false;
}

void WSServer::onNewConnection()
{
    QWebSocket* newSocket = wsServer->nextPendingConnection();
    sInfo() << "New connection from " << newSocket->origin();
    if (!trustedOrigin.contains(newSocket->origin()))
    {
        sInfo() << "Connection from untrusted origin";
        emit untrustedConnection(newSocket->origin());
        sInfo() << "Closing Connection";
        newSocket->close(QWebSocketProtocol::CloseCodePolicyViolated, "Not in trusted origin");
        return ;
    }

    connect(newSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onTextMessageReceived(QString)));
    connect(newSocket, SIGNAL(binaryMessageReceived(QByteArray)), this, SLOT(onBinaryMessageReceived(QByteArray)));
    connect(newSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    connect(newSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onClientError(QAbstractSocket::SocketError)));

    WSInfos wi;
    wi.name = "Websocket " + QString::number((size_t)newSocket, 16);
    wi.attached = false;
    wi.attachedTo = nullptr;
    wi.commandState = NOCOMMAND;
    wi.byteReceived = 0;
    wi.pendingAttach = false;
    wi.recvData.clear();
    wi.ipsSize = 0;
    wsInfos[newSocket] = wi;
    sInfo() << "New connection accepted " << wi.name << newSocket->origin() << newSocket->peerAddress();
}

void WSServer::onWSError(QWebSocketProtocol::CloseCode code)
{
    sDebug() << "Websocket error" << code;
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
    connect(device, &ADevice::commandFinished, this, &WSServer::onDeviceCommandFinished);
    connect(device, &ADevice::protocolError, this, &WSServer::onDeviceProtocolError);
    connect(device, &ADevice::closed, this, &WSServer::onDeviceClosed);
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

void WSServer::addDeviceFactory(DeviceFactory *devFact)
{
    sDebug() << "Adding Device Factory " << devFact->name();
    deviceFactories.append(devFact);
}

QStringList WSServer::getClientsName(ADevice *dev)
{
    QStringList toret;
    QMapIterator<QWebSocket*, WSInfos> wsIit(wsInfos);
    while (wsIit.hasNext())
    {
        auto p = wsIit.next();
        if (p.value().attachedTo == dev)
        {
            toret << p.value().name;
        }
    }
    return toret;
}


QList<WSServer::MiniDeviceInfos> WSServer::getDevicesInfo()
{
    QList<MiniDeviceInfos> toret;
    QListIterator<ADevice*> it(devices);
    QStringList canAttachDev = getDevicesList();

    while (it.hasNext())
    {
        MiniDeviceInfos mInfo;
        mInfo.usable = true;
        ADevice *dev = it.next();
        sDebug() << dev->name();
        mInfo.name = dev->name();
        if (!canAttachDev.contains(dev->name()))
        {
            mInfo.usable = false;
            mInfo.error = dev->attachError();
        } else {
            QMapIterator<QWebSocket*, WSInfos> wsIit(wsInfos);
            while (wsIit.hasNext())
            {
                auto p = wsIit.next();
                if (p.value().attachedTo == dev)
                {
                    mInfo.clients << p.value().name;
                }
            }
        }
        toret.append(mInfo);
    }
    /* SD2SNES devices are created only when attached */
    foreach (QString devName, canAttachDev)
    {
        bool pass = false;
        foreach(MiniDeviceInfos mDev, toret)
        {
            if (devName == mDev.name)
                pass = true;
        }
        if (pass)
            continue;
        MiniDeviceInfos mInfo;
        mInfo.name = devName;
        mInfo.usable = true;
        toret.append(mInfo);
    }
    return toret;
}

void WSServer::addTrusted(QString origin)
{
    trustedOrigin.append(origin);
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
            sDebug() << wsInfo.attachedTo->name() << "Adding request in queue (size:" << pendingRequests[wsInfo.attachedTo].size() << ")" << *req;
            addToPendingRequest(wsInfo.attachedTo, req);
            return ;
        } else {
            setError(ErrorType::ProtocolError, "Invalid command while unattached");
            goto LError;
        }
    }
    if (wsInfo.attached && !isValidUnAttached(req->opcode))
    {
        ADevice* dev = wsInfo.attachedTo;
        sDebug() << "Device is " << dev->state();
        if (dev->state() == ADevice::READY && pendingRequests[dev].isEmpty())
        {
            if ((isControlCommand(req->opcode) && !dev->hasControlCommands()) ||
                 (isFileCommand(req->opcode) && !dev->hasFileCommands()))
            {
                setError(ErrorType::DeviceError, QString("The device does not support the command %1").arg(cmdMetaEnum.valueToKey(static_cast<int>(req->opcode))));
                goto LError;
            }
            currentRequests[dev] = req;
            devicesInfos[dev].currentCommand = req->opcode;
            devicesInfos[dev].currentWS = ws;
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
    if (req->opcode == USB2SnesWS::PutAddress || req->opcode == USB2SnesWS::PutIPS)
    {
        bool ok;
        wsInfos[req->owner].pendingPutSizes.append(req->arguments.at(1).toInt(&ok, 16));
        wsInfos[req->owner].pendingPutReqWithNoData.append(req);
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
        if (infos.ipsSize != 0)
        {
            sDebug() << "Data sent are IPS data";
            infos.ipsData.append(data);
            if (infos.ipsData.size() == infos.ipsSize)
            {
                infos.recvData.clear();
                infos.byteReceived = 0;
                infos.ipsSize = 0;
                processIpsData(ws);
            }
            return ;
        }
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
                pend.removeFirst();
                infos.pendingPutReqWithNoData.removeFirst();
                infos.byteReceived = 0;
                infos.recvData.clear();
            }
        }

    }
}

void WSServer::onClientDisconnected()
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sInfo() << "Websocket disconnected" << wsInfos.value(ws).name;
    cleanUpSocket(ws);
}

void WSServer::onClientError(QAbstractSocket::SocketError)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sInfo() << "Client error : " << wsInfos.value(ws).name << ws->errorString();
}

void WSServer::onDeviceCommandFinished()
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos[device].currentWS != nullptr)
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
    sInfo() << "Device Error" << device->name();
    setError(ProtocolError, "Error in device protocol");
    disconnect(device, &ADevice::closed, this, &WSServer::onDeviceClosed);
    cleanUpDevice(device);
}

void WSServer::onDeviceClosed()
{
    ADevice* dev = qobject_cast<ADevice*>(sender());
    sInfo() << "Device closed" << dev->name();
    disconnect(dev, &ADevice::closed, this, &WSServer::onDeviceClosed);
    cleanUpDevice(dev);
}

void WSServer::onDeviceGetDataReceived(QByteArray data)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos.value(device).currentWS == nullptr)
    {
        sDebug() << "NOOP Sending get data to nothing" << device->name();
        return ;
    }
    sDebug() << "Sending " << data.size() << "to" << wsInfos.value(devicesInfos[device].currentWS).name;
    devicesInfos[device].currentWS->sendBinaryMessage(data);
}

// Used for Get File
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
        // Request is no longer in queue, so expected data need to not go in queue
        // if not already here.
        if (req->opcode == USB2SnesWS::PutAddress)
        {
            WSInfos& wInfos = wsInfos[req->owner];
            if (wInfos.pendingPutReqWithNoData.contains(req))
            {
                wInfos.pendingPutReqWithNoData.removeFirst();
                wInfos.pendingPutSizes.removeFirst();
            }
        }
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
    req->owner = nullptr;
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
    sInfo() << "Error with a ws client " << wsInfos[ws].name << m_errorType << m_errorString;
    ws->close();
    emit error();
}


void    WSServer::cleanUpDevice(ADevice* device)
{
    sDebug() << "Cleaning up device " << device->name();
    DeviceInfos& devInfo = devicesInfos[device];
    QMapIterator<QWebSocket*, WSInfos> it(wsInfos);
    QList<QWebSocket*> toDiscard;
    while (it.hasNext())
    {
        it.next();
        if (it.value().attachedTo == device)
            toDiscard.append(it.key());
    }
    foreach (QWebSocket* ws, toDiscard)
    {
        setError(WSServer::DeviceError, "Device closed");
        clientError(ws);
    }
    if (devInfo.currentWS != nullptr)
        devInfo.currentWS = nullptr;
    DeviceFactory* devFact = mapDevFact[device];
    mapDevFact.remove(device);
    devFact->deleteDevice(device);
}

void WSServer::cleanUpSocket(QWebSocket *ws)
{
    WSInfos wInfo = wsInfos.value(ws);
    sDebug() << "Cleaning up wsocket" << wInfo.name;
    if (wInfo.attached)
    {
        ADevice*    dev = wInfo.attachedTo;
        MRequest*   req = currentRequests[dev];
        if (devicesInfos.value(dev).currentWS == ws)
            devicesInfos[dev].currentWS = nullptr;
        if (req != nullptr && req->owner == ws)
        {
            req->owner = nullptr;
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
    if (ws == nullptr)
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
