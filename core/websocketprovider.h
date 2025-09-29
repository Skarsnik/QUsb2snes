#ifndef WEBSOCKETPROVIDER_H
#define WEBSOCKETPROVIDER_H

#include "aclientprovider.h"

#include <QHostAddress>
#include <QObject>
#include <QStringLiteral>
#include <QWebSocketServer>

class WebSocketProvider : public AClientProvider
{
    Q_OBJECT
public:
    WebSocketProvider(QString wsname, QObject* parent = nullptr);
    bool listen(QHostAddress lAddress, quint16 port);
    QString errorString() const;
    void    close();

private:
    QWebSocketServer*   m_server;
    quint16             m_port;
    void                onNewConnection();
    void                onClosed();
    void                onServerError(QWebSocketProtocol::CloseCode code);

signals:
    void    closed();
    // AClientProvider interface
public:
    void deleteClient(AClient *client);
};

#endif // WEBSOCKETPROVIDER_H
