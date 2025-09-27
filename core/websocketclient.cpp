#include "types.h"
#include "websocketclient.h"

#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>

Q_LOGGING_CATEGORY(log_wsclient, "WSClient")
#define sDebug() qCDebug(log_wsclient) << name
#define sInfo() qCInfo(log_wsclient) << name

using namespace Core;

WebSocketClient::WebSocketClient(QWebSocket *socket, AClientProvider* parent = nullptr) : AClient(parent)
{
    m_socket = socket;
    name = "Websocket " + QString::number((size_t) socket, 16);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocketClient::onClientDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WebSocketClient::onWSError);
}

void WebSocketClient::close()
{
    m_socket->close();
}

QString WebSocketClient::origin() const
{
    return m_socket->origin();
}

void WebSocketClient::close(QWebSocketProtocol::CloseCode code, QString str)
{
    m_socket->close(code, str);
}

void WebSocketClient::onWSError(QAbstractSocket::SocketError error)
{

}

void WebSocketClient::onTextMessageReceived(QString message)
{
    static const QMetaObject &mo = USB2SnesWS::staticMetaObject;
    static const QMetaObject &mo2 = SD2Snes::staticMetaObject;

    sDebug() << "received " << message;
    MRequest    *req = new MRequest();
    int i = mo.indexOfEnumerator("opcode");
    QMetaEnum cmdMetaEnum = mo.enumerator(i);
    i = mo2.indexOfEnumerator("space");
    QMetaEnum spaceMetaEnum = mo2.enumerator(i);
    i = mo2.indexOfEnumerator("server_flags");
    QMetaEnum flagsMetaEnum = mo2.enumerator(i);
    req->state = RequestState::NEW;
    req->owner = this;
    QJsonDocument   jdoc = QJsonDocument::fromJson(message.toLatin1());
    QJsonObject job = jdoc.object();
    QString opcode = job["Opcode"].toString();
    if (cmdMetaEnum.keyToValue(qPrintable(opcode)) == -1)
    {
        delete req;
        emit errorOccured(ErrorType::ProtocolError, "Invalid OPcode send " + opcode);
        return ;
    }
    if (job.contains("Space"))
    {
        QString space = job["Space"].toString();

        if (spaceMetaEnum.keyToValue(qPrintable(space)) == -1)
        {
            delete req;
            emit errorOccured(ErrorType::ProtocolError, "Invalid Space send" + space);
            return ;
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
    emit newRequest(req);
}

void WebSocketClient::onBinaryMessageReceived(QByteArray data)
{
    emit binaryData(data);
}

void WebSocketClient::onClientDisconnected()
{

}


void WebSocketClient::sendData(QByteArray data)
{
    m_socket->sendBinaryMessage(data);
}

void WebSocketClient::sendReply(QStringList list)
{
    // We don't reply on these commands
    switch (currentOpcode) {
    case USB2SnesWS::Rename :
    case USB2SnesWS::Remove :
    case USB2SnesWS::MakeDir :
    case USB2SnesWS::PutFile :
    case USB2SnesWS::PutAddress :
    case USB2SnesWS::Menu :
    case USB2SnesWS::Reset :
    case USB2SnesWS::Boot :
    case USB2SnesWS::Attach:
        return ;
    default:
        break;
    }
    QJsonObject jObj;
    QJsonArray ja;

    foreach(QString s, list)
    {
        ja.append(QJsonValue(s));
    }
    jObj["Results"] = ja;
    sDebug() << ">>" << QJsonDocument(jObj).toJson();
    m_socket->sendTextMessage(QJsonDocument(jObj).toJson());
}

void WebSocketClient::sendError(ErrorType err, QString errString)
{
    sDebug() << "Error : " << err << errString;
    m_socket->close(QWebSocketProtocol::CloseCodeNormal, errString);
}
