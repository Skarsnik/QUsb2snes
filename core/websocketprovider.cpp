#include <QLoggingCategory>
#include "websocketclient.h"
#include "websocketprovider.h"

Q_LOGGING_CATEGORY(log_wsprovider, "WebSocketProvider")
#define sDebug() qCDebug(log_wsprovider)
#define sInfo() qCInfo(log_wsprovider)

WebSocketProvider::WebSocketProvider(QString wsname, QObject* parent) : AClientProvider(parent)
{
    m_server = new QWebSocketServer(wsname, QWebSocketServer::NonSecureMode, this);
    wsname = "WebSocket Server";
}

bool WebSocketProvider::listen(QHostAddress lAddress, quint16 port)
{
    if (!m_server->listen(lAddress, port))
        return false;
    m_port = port;
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketProvider::onNewConnection);
    connect(m_server, &QWebSocketServer::closed, this, &WebSocketProvider::onClosed);
    connect(m_server, &QWebSocketServer::serverError, this, &WebSocketProvider::onServerError);
    return true;
}

QString WebSocketProvider::errorString() const
{
    return m_server->errorString();
}

void WebSocketProvider::close()
{
    m_server->close();
}

void WebSocketProvider::onNewConnection()
{
    QWebSocket* newSocket = m_server->nextPendingConnection();
    WebSocketClient* newclient = new WebSocketClient(newSocket, this);
    emit newClient(newclient);
}

void WebSocketProvider::onClosed()
{
    sDebug() << "Webserver on port " << m_port << "Closed";
    emit closed();
}

void WebSocketProvider::onServerError(QWebSocketProtocol::CloseCode code)
{
    sDebug() << "Websocket error" << code;
}


void WebSocketProvider::deleteClient(AClient *client)
{
    client->deleteLater();
}
