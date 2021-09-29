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

private:
    QWebSocketServer*   m_server;
    void                onNewConnection();
    void                onClosed();
    void                onServerError();

    // AClientProvider interface
public:
    void deleteClient(AClient *client);
};

#endif // WEBSOCKETPROVIDER_H
