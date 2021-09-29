#include "types.h"
#include "websocketclient.h"

#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>

Q_LOGGING_CATEGORY(log_wsclient, "WSClient")
#define sDebug() qCDebug(log_wsclient)
#define sInfo() qCInfo(log_wsclient)

using namespace Core;

WebSocketClient::WebSocketClient(QWebSocket *socket, QObject *parent = nullptr) : AClient(parent)
{
    m_socket = socket;
    name = "Websocket " + QString::number((size_t) socket, 16);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived);
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWSError(QAbstractSocket::SocketError)));
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

void WebSocketClient::onWSError(QWebSocketProtocol::CloseCode code)
{

}

void WebSocketClient::onTextMessageReceived(QString message)
{
    sDebug() << name << "received " << message;
    MRequest    *req = new MRequest();
    const QMetaObject &mo = USB2SnesWS::staticMetaObject;
    const QMetaObject &mo2 = SD2Snes::staticMetaObject;
    int i = mo.indexOfEnumerator("opcode");
    QMetaEnum cmdMetaEnum = mo.enumerator(i);
    i = mo2.indexOfEnumerator("space");
    QMetaEnum spaceMetaEnum = mo2.enumerator(i);
    i = mo2.indexOfEnumerator("server_flags");
    QMetaEnum flagsMetaEnum = mo2.enumerator(i);
    req->state = RequestState::NEW;
    req->owner = nullptr;
    QJsonDocument   jdoc = QJsonDocument::fromJson(message.toLatin1());
    QJsonObject job = jdoc.object();
    QString opcode = job["Opcode"].toString();
    if (cmdMetaEnum.keyToValue(qPrintable(opcode)) == -1)
    {
        free(req);
        emit errorOccured(ErrorType::ProtocolError, "Invalid OPcode send " + opcode);
        return ;
    }
    if (job.contains("Space"))
    {
        QString space = job["Space"].toString();

        if (spaceMetaEnum.keyToValue(qPrintable(space)) == -1)
        {
            free(req);
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


void WebSocketClient::sendData(QByteArray data)
{
}

void WebSocketClient::sendReply(QStringList list)
{
}

void WebSocketClient::setError(ErrorType err, QString errString)
{
}
