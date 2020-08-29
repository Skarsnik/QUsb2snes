#ifndef WSSERVER_H
#define WSSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QDebug>
#include <QLoggingCategory>
#include <QMetaEnum>
#include "adevice.h"
#include "devicefactory.h"

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
        DONE,
        CANCELLED
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
        unsigned int            currentPutSize;
        QList<unsigned int>     pendingPutSizes;
        QList<QByteArray>       pendingPutDatas;
        QByteArray              recvData;
        QByteArray              ipsData;
        unsigned int            ipsSize;
        unsigned int            byteReceived;
        bool                    pendingAttach;
    };

    struct DeviceInfos {
        QWebSocket*         currentWS;
        USB2SnesWS::opcode  currentCommand;
    };

public:
    struct MiniDeviceInfos {
       QString  name;
       bool     usable;
       QString  error;
       QStringList  clients;
    };

    explicit    WSServer(QObject *parent = nullptr);
    QString     start(QHostAddress lAddress, quint16 port);
    QString&    errorString() const;
    void        addDevice(ADevice* device);
    void        removeDevice(ADevice* device);
    void        addDeviceFactory(DeviceFactory* devFact);
    QStringList getClientsName(ADevice* dev);
    QList<MiniDeviceInfos>  getDevicesInfo();
    void        addTrusted(QString origin);

signals:
    void    error();
    void    untrustedConnection(QString origin);
    void    listenFailed(QString err);

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
    QMetaEnum                           flagsMetaEnum;
    QList<QWebSocketServer*>            wsServers;
    //QWebSocketServer*                   wsServer;
    QMap<QWebSocket*, WSInfos>          wsInfos;
    QList<ADevice*>                     devices;
    QList<DeviceFactory*>               deviceFactories;
    QMap<ADevice*, DeviceFactory*>      mapDevFact;
    QMap<ADevice*, DeviceInfos>         devicesInfos;
    QMap<ADevice*, MRequest*>           currentRequests;
    QString                             m_errorString;
    ErrorType                           m_errorType;
    QStringList                         deviceList;
    QStringList                         trustedOrigin;

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
    void        cmdAttach(MRequest* req);
    void        processIpsData(QWebSocket* ws);
    void        sendReply(QWebSocket* ws, const QStringList& args);
    void        sendReply(QWebSocket* ws, QString args);


    void cmdInfo(MRequest *req);
    bool isFileCommand(USB2SnesWS::opcode opcode);
    bool isControlCommand(USB2SnesWS::opcode opcode);
    void addToPendingRequest(ADevice *device, MRequest *req);
    void cleanUpDevice(ADevice *device);
};

#endif // WSSERVER_H
