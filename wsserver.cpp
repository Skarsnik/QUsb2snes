#include "wsserver.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_wsserver, "WSServer")
#define sDebug() qCDebug(log_wsserver)


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
    const WSInfos &wsInfo = wsInfos[ws];
    sDebug() << wsNames[ws] << "received " << message;

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
        }
        else // add to queue
        {
            sDebug() << wsInfo.attachedTo->name() << "Adding request in queue " << req->opcode << "(" << pendingRequests[wsInfo.attachedTo].size() << ")";
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

}

void WSServer::onClientDisconnected()
{
    QWebSocket* ws = qobject_cast<QWebSocket*>(sender());
    sDebug() << "Websocket disconnected" << wsNames.value(ws);
    cleanUpSocket(ws);
}

void WSServer::onLowCoCommandFinished()
{
    USBConnection*  usbco = qobject_cast<USBConnection*>(sender());
    processLowCoCmdFinished(usbco);
}

void WSServer::onLowCoProtocolError()
{

}

void WSServer::onLowCoClosed()
{

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
            lowConnectionInfos.remove(i.key());
            break;
        }
    }
    delete ws;
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
    debug << "Created at" << req.timeCreated << "-" << req.opcode << req.space << req.flags << req.arguments;
    return debug;
}
