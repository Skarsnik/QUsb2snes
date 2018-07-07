#ifndef WSSERVER_H
#define WSSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QDebug>
#include <QLoggingCategory>
#include <QMetaEnum>
#include "usbconnection.h"

Q_DECLARE_LOGGING_CATEGORY(log_wsserver)

/*
 * IMPORTANT : Implementation is on 2 files
 * Commands are in wsservercommand.cpp to keep only the server logic on wsserver.cpp
*/

class WSServer : public QObject
{
    Q_OBJECT
public:
    enum ClientCommandState {
        NOCOMMAND,
        WAITINGREPLY,
        WAITINGBDATAREPLY
    };
    Q_ENUM(ClientCommandState)

    enum ErrorType {
        CommandError,
        ProtocolError,
        DeviceError,
    };
    Q_ENUM(ErrorType)

    enum class RequestState {
        NEW,
        SENT,
        WAITINGREPLY,
        DONE
    };
    Q_ENUM(RequestState)
private:

    struct MRequest {
        QWebSocket*         owner;
        QTime               timeCreated;
        USB2SnesWS::opcode  opcode;
        QString             space;
        QStringList         arguments;
        QStringList         flags;
        RequestState        state;
        friend QDebug              operator<<(QDebug debug, const MRequest& req);
    };
    friend QDebug              operator<<(QDebug debug, const WSServer::MRequest& req);
    struct WSInfos {
        bool                attached;
        USBConnection*      attachedTo;
        ClientCommandState  commandState;
    };

    struct LowCoInfos {
        QWebSocket*         currentWS;
        USB2SnesWS::opcode  currentCommand;
    };

public:
    explicit WSServer(QObject *parent = nullptr);
    bool     start();
    QString&  errorString() const;

signals:
    error();

public slots:

private slots:
    void    onNewConnection();
    void    onWSError(QWebSocketProtocol::CloseCode code);
    void    onWSClosed();
    void    onTextMessageReceived(QString message);
    void    onBinaryMessageReceived(QByteArray data);
    void    onClientDisconnected();
    void    onLowCoCommandFinished();
    void    onLowCoProtocolError();
    void    onLowCoClosed();

private:
    QMetaEnum                           cmdMetaEnum;
    QWebSocketServer*                   wsServer;
    QList<QWebSocket*>                  unassignedWS;
    QMap<QWebSocket*, WSInfos>          wsInfos;
    QList<USBConnection*>               lowConnections;
    QMap<USBConnection*, LowCoInfos>    lowConnectionInfos;
    QMap<USBConnection*, MRequest*>     currentRequest;
    QMap<QWebSocket*, QString>          wsNames;
    QString                             m_errorString;
    ErrorType                           m_errorType;
    QStringList                         deviceList;

    QMap<USBConnection*, QList<MRequest*> >    pendingRequests;

    void        setError(const ErrorType type, const QString reason);
    MRequest    *requestFromJSON(const QString& str);
    void        clientError(QWebSocket* ws);
    void        cleanUpSocket(QWebSocket* ws);
    bool        isValidUnAttached(const USB2SnesWS::opcode opcode);
    void        executeRequest(MRequest *req);
    void        addNewLowConnection(USBConnection* lowCo);
    void        processLowCoCmdFinished(USBConnection* usbco);

    QStringList getDevicesList();
    void        cmdAttach(MRequest *req);
    void        sendReply(QWebSocket* ws, const QStringList& args);
    void        sendReply(QWebSocket* ws, QString args);


    void cmdInfo(MRequest *req);
};

#endif // WSSERVER_H
