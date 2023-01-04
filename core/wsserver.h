/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

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
#include "types.h"
#include "aclient.h"
#include "websocketprovider.h"
#include "websocketclient.h"

using namespace Core;

Q_DECLARE_LOGGING_CATEGORY(log_wsserver)

/*
 * IMPORTANT : Implementation is on 2 files
 * Commands are in wsservercommand.cpp to keep only the server logic on wsserver.cpp
*/

class WSServer : public QObject
{
    Q_OBJECT
private:

    struct DeviceInfos {
        AClient*            currentClient;
        USB2SnesWS::opcode  currentCommand;
    };

public:
    struct MiniDeviceInfos {
       QString  name;
       bool     usable;
       QString  error;
       QStringList  clients;
    };

    struct ServerStatus {
        int clientCount;
        int deviceCount;
        int deviceFactoryCount;
    };

    explicit    WSServer(QObject *parent = nullptr);
    QString     start(QHostAddress lAddress, quint16 port);
    QString&    errorString() const;
    void        addDevice(ADevice* device);
    void        removeDevice(ADevice* device);
    void        addDeviceFactory(DeviceFactory* devFact);
    QStringList deviceFactoryNames() const;
    void        removeDeviceFactory(DeviceFactory* devFact);
    QStringList getClientsName(ADevice* dev);
    QStringList getClientsName(const QString devName) const;
    QList<MiniDeviceInfos>  getDevicesInfo();
    void        addTrusted(QString origin);
    ServerStatus  serverStatus() const;
    void        requestDeviceStatus();

signals:
    void    error();
    void    untrustedConnection(QString origin);
    void    listenFailed(QString err);
    void    newDeviceFactoryStatus(DeviceFactory::DeviceFactoryStatus status);
    void    deviceFactoryStatusDone();

public slots:

private slots:
    void    onNewWsClient(AClient* client);
    void    onNewRequest(MRequest* req);
    void    onBinaryMessageReceived(QByteArray data);
    void    onClientDisconnected();
    void    onClientError();
    void    onDeviceCommandFinished();
    void    onDeviceProtocolError();
    void    onDeviceClosed();
    void    onDeviceGetDataReceived(QByteArray data);
    void    onDeviceSizeGet(unsigned int size);
    void    onNewDeviceName(QString name);
    void    onDeviceListDone();
    void    onDeviceFactoryStatusDone(DeviceFactory::DeviceFactoryStatus);

private:
    QMetaEnum                           cmdMetaEnum;
    QMetaEnum                           spaceMetaEnum;
    QMetaEnum                           flagsMetaEnum;
    QList<WebSocketProvider*>           wsServers;
    QList<ADevice*>                     devices; // Mostly used to keep tracks of signal/slots connection
    QList<DeviceFactory*>               deviceFactories;
    QList<AClient*>                     clients;
    QMap<ADevice*, DeviceFactory*>      mapDevFact;
    QMap<ADevice*, DeviceInfos>         devicesInfos;
    QMap<ADevice*, MRequest*>           currentRequests;
    QString                             m_errorString;
    ErrorType                           m_errorType;
    QStringList                         trustedOrigin;

    // Used for the async devicelist stuff
    unsigned    int                     numberOfAsyncFactory;
    unsigned    int                     pendingDeviceListQuery;
    QList<AClient*>                     pendingDeviceListClients;
    QList<MRequest*>                    pendingDeviceListRequests;
    QStringList                         deviceList;

    QMap<ADevice*, QList<MRequest*> >   pendingRequests;

    int                                 factoryStatusCount;
    int                                 factoryStatusDoneCount;

    void        setError(const ErrorType type, const QString reason);
    MRequest*   requestFromJSON(const QString& str);
    void        clientError(AClient* client);
    void        cleanUpClient(AClient* client);
    bool        isValidUnAttached(const USB2SnesWS::opcode opcode);
    void        executeRequest(MRequest* req);
    void        executeServerRequest(MRequest *req);
    void        processDeviceCommandFinished(ADevice* device);
    Q_INVOKABLE void        processCommandQueue(ADevice* device);

    void        asyncDeviceList();
    QStringList getDevicesList();
    void        cmdAttach(MRequest* req);
    void        processIpsData(AClient* client);
    void        sendReply(AClient* client, const QStringList& args);
    void        sendReply(AClient* client, QString args);


    void    cmdInfo(MRequest *req);
    bool    isV2WebSocket(QWebSocket *ws);
    bool    isFileCommand(USB2SnesWS::opcode opcode);
    bool    isControlCommand(USB2SnesWS::opcode opcode);
    void    addToPendingRequest(ADevice *device, MRequest *req);
    void    cleanUpDevice(ADevice *device);
    void    sendError(QWebSocket *ws, ErrorType errType, QString errorString);

};

#endif // WSSERVER_H
