#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include "aclient.h"
#include <QWebSocket>

class WebSocketClient : public AClient
{
    Q_OBJECT
public:
    WebSocketClient(QWebSocket* socket, AClientProvider* parent);

public:
    void close();
    QString origin() const;
    void close(QWebSocketProtocol::CloseCode code, QString str);

private:
    QWebSocket  *m_socket;

private slots:
    void    onWSError(QAbstractSocket::SocketError error);
    void    onTextMessageReceived(QString message);
    void    onBinaryMessageReceived(QByteArray data);
    void    onClientDisconnected();

    // AClient interface
public:
    void sendData(QByteArray data);
    void sendReply(QStringList list);
    void sendError(Core::ErrorType err, QString errString);
};

#endif // WEBSOCKETCLIENT_H
