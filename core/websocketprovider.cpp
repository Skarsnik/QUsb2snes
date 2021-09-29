#include "websocketclient.h"
#include "websocketprovider.h"

WebSocketProvider::WebSocketProvider(QString wsname, QObject* parent) : AClientProvider(parent)
{
    m_server = new QWebSocketServer(wsname, QWebSocketServer::NonSecureMode, this);
    wsname = "WebSocket Server";
}

bool WebSocketProvider::listen(QHostAddress lAddress, quint16 port)
{
    if (!m_server->listen(lAddress, port))
        return false;
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketProvider::onNewConnection);
    connect(m_server, &QWebSocketServer::closed, this, &WebSocketProvider::onClosed);
    connect(m_server, &QWebSocketServer::serverError, this, &WebSocketProvider::onServerError);
}

QString WebSocketProvider::errorString() const
{
    return m_server->errorString();
}

void WebSocketProvider::onNewConnection()
{
    QWebSocket* newSocket = m_server->nextPendingConnection();
    WebSocketClient* newclient = new WebSocketClient(newSocket, this);
    emit newClient(newclient);
}

void WebSocketProvider::onClosed()
{

}

void WebSocketProvider::onServerError()
{

}


void WebSocketProvider::deleteClient(AClient *client)
{
}
