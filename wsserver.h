#ifndef WSSERVER_H
#define WSSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QDebug>
#include <QLoggingCategory>
#include <QMetaEnum>
#include "adevice.h"
#include "retroarchdevice.h"

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
        WAITINGBDATAREPLY,
        DONE
    };
    Q_ENUM(RequestState)
private:

    struct MRequest {
        MRequest() {
            id = gId++;
            wasPending = false;
        }
        quint64             id;
        QWebSocket*         owner;
        QTime               timeCreated;
        USB2SnesWS::opcode  opcode;
        SD2Snes::space      space;
        QStringList         arguments;
        QStringList         flags;
        RequestState        state;
        bool                wasPending;
        friend QDebug              operator<<(QDebug debug, const MRequest& req);
    private:
        static quint64      gId;
    };

    friend QDebug              operator<<(QDebug debug, const WSServer::MRequest& req);
    struct WSInfos {
        QString                 name;
        bool                    attached;
        ADevice*                attachedTo;
        ClientCommandState      commandState;
        QList<unsigned int>     pendingPutSizes;
        QList<QByteArray>       pendingPutDatas;
        QByteArray              recvData;
        unsigned int            byteReceived;

    };

    struct DeviceInfos {
        QWebSocket*         currentWS;
        USB2SnesWS::opcode  currentCommand;
    };

public:
    explicit WSServer(QObject *parent = nullptr);
    bool     start();
    QString&  errorString() const;
    void        addDevice(ADevice* device);
    void        removeDevice(ADevice* device);
    QMap<QString, QStringList>  getDevicesInfo();

signals:
    void    error();

public slots:

private slots:
    void    onNewConnection();
    void    onWSError(QWebSocketProtocol::CloseCode code);
    void    onWSClosed();
    void    onTextMessageReceived(QString message);
    void    onBinaryMessageReceived(QByteArray data);
    void    onClientDisconnected();
    void    onClientError(QAbstractSocket::SocketError);
    void    onDeviceCommandFinished();
    void    onDeviceProtocolError();
    void    onDeviceClosed();
    void    onDeviceGetDataReceived(QByteArray data);
    void    onDeviceSizeGet(unsigned int size);

private:
    QMetaEnum                           cmdMetaEnum;
    QMetaEnum                           spaceMetaEnum;
    QWebSocketServer*                   wsServer;
    QMap<QWebSocket*, WSInfos>          wsInfos;
    QList<ADevice*>                     devices;
    QMap<ADevice*, DeviceInfos>         devicesInfos;
    QMap<ADevice*, MRequest*>           currentRequests;
    QString                             m_errorString;
    ErrorType                           m_errorType;
    QStringList                         deviceList;

    QMap<ADevice*, QList<MRequest*> >   pendingRequests;

    void        setError(const ErrorType type, const QString reason);
    MRequest*   requestFromJSON(const QString& str);
    void        clientError(QWebSocket* ws);
    void        cleanUpSocket(QWebSocket* ws);
    bool        isValidUnAttached(const USB2SnesWS::opcode opcode);
    void        executeRequest(MRequest* req);
    void        processDeviceCommandFinished(ADevice* device);
    void        processCommandQueue(ADevice* device);

    QStringList getDevicesList();
    void        cmdAttach(MRequest *req);
    void        sendReply(QWebSocket* ws, const QStringList& args);
    void        sendReply(QWebSocket* ws, QString args);


    void cmdInfo(MRequest *req);
    bool isFileCommand(USB2SnesWS::opcode opcode);
    bool isControlCommand(USB2SnesWS::opcode opcode);
    void addToPendingRequest(ADevice *device, MRequest *req);
};

#endif // WSSERVER_H
