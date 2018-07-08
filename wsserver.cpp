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
    int i = mo.indexOfEnumerator("opcode");
    cmdMetaEnum = mo.enumerator(i);
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

void WSServer::addNewLowConnection(USBConnection *lowCo)
{
    lowConnections.append(lowCo);
    connect(lowCo, SIGNAL(commandFinished()), this, SLOT(onLowCoCommandFinished()));
    connect(lowCo, SIGNAL(protocolError()), this, SLOT(onLowCoProtocolError()));
    connect(lowCo, SIGNAL(closed()), this, SLOT(onLowCoClosed()));
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
        if (wsInfo.attachedTo->state() == USBConnection::READY)
        {
            USBConnection* lowCo = wsInfo.attachedTo;
            currentRequest[lowCo] = req;
            lowConnectionInfos[lowCo].currentCommand = req->opcode;
            lowConnectionInfos[lowCo].currentWS = ws;
            executeRequest(req);
        } else { // add to queue
            sDebug() << wsInfo.attachedTo->name() << "Adding request in queue " << *req << "(" << pendingRequests[wsInfo.attachedTo].size() << ")";
            pendingRequests[wsInfo.attachedTo].append(req);
        }

    }
    if (isValidUnAttached(req->opcode))
        executeRequest(req);
    return ;
LError:
    clientError(ws);
}

void WSServer::onBinaryMessageReceived(QByteArray data)
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    WSInfos infos = wsInfos.value(ws);
    sDebug() << wsNames.value(ws) << "Received binary data" << data.size();
    if (infos.commandState != ClientCommandState::WAITINGBDATAREPLY)
    {
        setError(ErrorType::ProtocolError, "Sending binary data when nothing waiting for it");
        clientError(ws);
    } else {
        wsInfos.value(ws).attachedTo->writeData(data);
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

void WSServer::onLowCoCommandFinished()
{
    USBConnection*  usbco = qobject_cast<USBConnection*>(sender());
    if (lowConnectionInfos[usbco].currentWS != NULL)
    {
        processLowCoCmdFinished(usbco);
        processCommandQueue(usbco);
    }
    else
        sDebug() << "Received finished command while no socket to receive it";
}

void WSServer::onLowCoProtocolError()
{

}

void WSServer::onLowCoClosed()
{

}

void WSServer::onLowCoGetDataReceived(QByteArray data)
{
    USBConnection*  usbco = qobject_cast<USBConnection*>(sender());
    sDebug() << "Sending " << data.size() << "to" << wsNames.value(lowConnectionInfos[usbco].currentWS);
    lowConnectionInfos[usbco].currentWS->sendBinaryMessage(data);
}

void WSServer::onLowCoSizeGet(unsigned int size)
{
    USBConnection*  usbco = qobject_cast<USBConnection*>(sender());
    sendReply(lowConnectionInfos[usbco].currentWS, QString::number(size, 16));
}

void        WSServer::processCommandQueue(USBConnection* usbco)
{
    QList<MRequest*>&    cmdQueue = pendingRequests[usbco];
    if (!cmdQueue.isEmpty())
    {
        sDebug() << cmdQueue.size() << " requests in queue, processing the first";
        MRequest* req = cmdQueue.takeFirst();
        currentRequest[usbco] = req;
        lowConnectionInfos[usbco].currentCommand = req->opcode;
        lowConnectionInfos[usbco].currentWS = req->owner;
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
        req->owner = (QWebSocket*) 42;
        setError(ErrorType::ProtocolError, "Invalid OPcode send" + opcode);
        return req;
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
    if (job.contains("Space"))
        req->space = job["Space"].toString();
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
    QMapIterator<USBConnection*, LowCoInfos> i(lowConnectionInfos);
    while (i.hasNext())
    {
        i.next();
        if (i.value().currentWS == ws)
        {
            i.value().currentWS == NULL;
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
