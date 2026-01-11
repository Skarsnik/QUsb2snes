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

#include "wsserver.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMetaObject>
#include <QSettings>
#include "websocketclient.h"

Q_LOGGING_CATEGORY(log_wsserver, "WSServer")
#define sDebug() qCDebug(log_wsserver)
#define sInfo() qCInfo(log_wsserver)

extern QSettings*          globalSettings;

quint64 MRequest::gId = 0;

WSServer::WSServer(QObject *parent) : QObject(parent)
{
    const QMetaObject &mo = USB2SnesWS::staticMetaObject;
    const QMetaObject &mo2 = SD2Snes::staticMetaObject;
    int i = mo.indexOfEnumerator("opcode");
    cmdMetaEnum = mo.enumerator(i);
    i = mo2.indexOfEnumerator("space");
    spaceMetaEnum = mo2.enumerator(i);
    i = mo2.indexOfEnumerator("server_flags");
    flagsMetaEnum = mo2.enumerator(i);
    trustedOrigin.append("http://localhost");
    trustedOrigin.append(""); // why?
    numberOfAsyncFactory = 0;
    remoteFactory = nullptr;
}

QString WSServer::start(QHostAddress lAddress, quint16 port)
{
    WebSocketProvider* newServer = new WebSocketProvider("USB2SNES Server", this);
    if (newServer->listen(lAddress, port))
    {
        sInfo() << "WebSocket server started : listenning " << lAddress << "port : " << port;
        connect(newServer, &WebSocketProvider::newClient, this, &WSServer::onNewWsClient);
        connect(newServer, &WebSocketProvider::closed, this, &WSServer::onWSClosed);
        wsServers[port] = newServer;
        return QString();
    }
    QString err = newServer->errorString();
    newServer->deleteLater();
    return err;
}

void    WSServer::stop(quint16 port)
{
    if (!wsServers.contains(port))
        return;
    wsServers[port]->close();
    sInfo() << "Closing Websocket server running on port " << port;
}

void WSServer::onNewWsClient(AClient* client)
{
    WebSocketClient* wsClient = qobject_cast<WebSocketClient*>(client);
    sInfo() << "New WS Connection from " << wsClient->origin();
    if (!trustedOrigin.contains(wsClient->origin()))
    {
        sInfo() << "Connection from untrusted origin";
        emit untrustedConnection(wsClient->origin());
        sInfo() << "Closing Connection";
        wsClient->close(QWebSocketProtocol::CloseCodePolicyViolated, "Not in trusted origin");
        return ;
    }
    onNewClient(client);
    sInfo() << "New connection accepted " << client->name << wsClient->origin();
}

void    WSServer::onNewClient(AClient* client)
{
    connect(client, &AClient::newRequest, this, &WSServer::onNewRequest);
    connect(client, &AClient::binaryData, this, &WSServer::onBinaryDataReceivedFromClient);
    connect(client, &AClient::closed, this, &WSServer::onClientDisconnected);
    client->attached = false;
    client->attachedTo = nullptr;
    client->commandState = AClient::ClientCommandState::NOCOMMAND;
    client->byteReceived = 0;
    client->pendingAttach = false;
    client->recvData.clear();
    client->expectedDataSize = 0;
    client->ipsSize = 0;
    clients.append(client);
}

void WSServer::onWSClosed()
{
    WebSocketProvider* server = qobject_cast<WebSocketProvider*>(sender());
    quint16 port = wsServers.key(server);
    if (port == 0) // Why?
        return;
    wsServers[port]->deleteLater();
    wsServers.remove(port);
    sInfo() << "Websocket server on "<< port << " closed";
}

void WSServer::addDevice(ADevice *device)
{
    sDebug() << "Adding device" << device->name();
    devices.append(device);
    devicesInfos[device] = DeviceInfos();
    connect(device, &ADevice::commandFinished, this, &WSServer::onDeviceCommandFinished);
    connect(device, &ADevice::protocolError, this, &WSServer::onDeviceProtocolError);
    connect(device, &ADevice::closed, this, &WSServer::onDeviceClosed);
    sDebug() << "Added device : " << device->name();
}

void WSServer::removeDevice(ADevice *device)
{
    setError(ErrorType::DeviceError, "Manualy removed the device (probably from the UI");
    cleanUpDevice(device);
    devices.removeAt(devices.indexOf(device));
    devicesInfos.remove(device);

    // TODO remove and disconnect WS
}

void WSServer::addDeviceFactory(DeviceFactory *devFact)
{
    sDebug() << "Adding Device Factory " << devFact->name();
    connect(devFact, &DeviceFactory::deviceStatusDone, this, &WSServer::onDeviceFactoryStatusDone);
    if (devFact->hasAsyncListDevices())
    {
        numberOfAsyncFactory++;
        connect(devFact, &DeviceFactory::newDeviceName, this, &WSServer::onNewDeviceName);
        connect(devFact, &DeviceFactory::devicesListDone, this, &WSServer::onDeviceListDone);
    }
    deviceFactories.append(devFact);
    if (qobject_cast<RemoteUsb2SnesWFactory*>(devFact) != nullptr)
    {
        remoteFactory = qobject_cast<RemoteUsb2SnesWFactory*>(devFact);
    }
}

QStringList WSServer::deviceFactoryNames() const
{
    QStringList toret;
    for (auto devFact : deviceFactories)
    {
        toret << devFact->name();
    }
    return toret;
}

// TODO: this is bad if you do it during a devicelist request x)

void WSServer::removeDeviceFactory(DeviceFactory *devFact)
{
    sDebug() << "Removing Device Factory" << devFact->name();
    if (devFact->hasAsyncListDevices())
    {
        numberOfAsyncFactory--;
    }
    disconnect(devFact, nullptr, this, nullptr);
    deviceFactories.removeAll(devFact);
    if (qobject_cast<RemoteUsb2SnesWFactory*>(devFact) != nullptr)
    {
        remoteFactory = nullptr;
    }
}

QStringList WSServer::getClientsName(ADevice *dev)
{
    QStringList toret;
    for(AClient* cl : clients)
    {
        if (cl->attached && cl->attachedTo == dev)
        {
            toret << cl->name;
        }
    }
    return toret;
}

QStringList WSServer::getClientsName(const QString devName) const
{
    QStringList toret;
    for(AClient* cl : clients)
    {
        if (cl->attached && cl->attachedTo->name() == devName)
        {
            toret << cl->name;
        }
    }
    return toret;
}

QList<WSServer::MiniDeviceInfos> WSServer::getDevicesInfo()
{
    QList<MiniDeviceInfos> toret;
    QListIterator<ADevice*> it(devices);
    QStringList canAttachDev = getDevicesList();

    while (it.hasNext())
    {
        MiniDeviceInfos mInfo;
        mInfo.usable = true;
        ADevice *dev = it.next();
        sDebug() << dev->name();
        mInfo.name = dev->name();
        if (!canAttachDev.contains(dev->name()))
        {
            mInfo.usable = false;
            mInfo.error = dev->attachError();
        } else {
            for (AClient* client : clients)
            {
                if (client->attachedTo == dev)
                {
                    mInfo.clients << client->name;
                }
            }
        }
        toret.append(mInfo);
    }
    /* SD2SNES devices are created only when attached */
    foreach (QString devName, canAttachDev)
    {
        bool pass = false;
        foreach(MiniDeviceInfos mDev, toret)
        {
            if (devName == mDev.name)
                pass = true;
        }
        if (pass)
            continue;
        MiniDeviceInfos mInfo;
        mInfo.name = devName;
        mInfo.usable = true;
        toret.append(mInfo);
    }
    return toret;
}

void WSServer::addTrusted(QString origin)
{
    trustedOrigin.append(origin);
}

WSServer::ServerStatus WSServer::serverStatus() const
{
    WSServer::ServerStatus status;
    status.clientCount = clients.size();
    status.deviceFactoryCount = deviceFactories.size();
    status.deviceCount = devices.size();
    return status;
}

void WSServer::requestDeviceStatus()
{
    factoryStatusCount = 0;
    factoryStatusDoneCount = 0;
    for (DeviceFactory* devFac : qAsConst(deviceFactories))
    {
        if (devFac->devicesStatus())
            factoryStatusCount++;
    }
    sDebug() << "Number of factory to check" << factoryStatusCount;
    if (factoryStatusCount == 0)
        emit deviceFactoryStatusDone();
}

QStringList WSServer::getAllClientsName()
{
    QStringList toret;
    for (const auto& client : clients)
    {
        toret.append(client->name);
    }
    return toret;
}

void WSServer::onDeviceFactoryStatusDone(DeviceFactory::DeviceFactoryStatus status)
{
    factoryStatusDoneCount++;
    sDebug() << "DeviceFactory status done : " << factoryStatusDoneCount << "/" << factoryStatusCount;
    emit newDeviceFactoryStatus(status);
    if (factoryStatusCount == factoryStatusDoneCount)
        emit deviceFactoryStatusDone();
}


void WSServer::onNewRequest(MRequest* req)
{
    AClient* client = req->owner;
    sDebug() << client->name << "received " << *req;
    if (client->attached == false && !isValidUnAttached(req->opcode))
    {
        if (client->pendingAttach)
        {
            sDebug() << client->attachedTo->name() << "Adding request in queue (size:" << pendingRequests[client->attachedTo].size() << ")" << *req;
            addToPendingRequest(client->attachedTo, req);
            return ;
        } else {
            client->sendError(ErrorType::ProtocolError, "Invalid command while unattached");
            return;
        }
    }
    if (client->attached && !isValidUnAttached(req->opcode))
    {
        ADevice* dev = client->attachedTo;
        sDebug() << "Device is " << dev->state();
        if (dev->state() == ADevice::READY && pendingRequests[dev].isEmpty())
        {
            if ((isControlCommand(req->opcode) && !dev->hasControlCommands()) ||
                (isFileCommand(req->opcode) && !dev->hasFileCommands()))
            {
                client->sendError(ErrorType::DeviceError, QString("The device does not support the command %1").arg(cmdMetaEnum.valueToKey(static_cast<int>(req->opcode))));
                return ;
            }
            currentRequests[dev] = req;
            devicesInfos[dev].currentCommand = req->opcode;
            devicesInfos[dev].currentClient = client;
            executeRequest(req);
        } else { // add to queue
            sDebug() << client->attachedTo->name() << "Adding request in queue " << *req << "(" << pendingRequests[client->attachedTo].size() << ")";
            addToPendingRequest(client->attachedTo, req);
        }
    }
    if (isValidUnAttached(req->opcode))
        executeServerRequest(req);
    return ;
}

void    WSServer::addToPendingRequest(ADevice* device, MRequest *req)
{
    pendingRequests[device].append(req);
    if (req->opcode == USB2SnesWS::PutAddress || req->opcode == USB2SnesWS::PutIPS || req->opcode == USB2SnesWS::PutFile)
    {
        bool ok;
        unsigned putSize = req->arguments.at(1).toUInt(&ok, 16);
        if (req->opcode == USB2SnesWS::PutAddress && req->arguments.size() > 3)
        {
            putSize = 0;
            for (int i = 0; i < req->arguments.size(); i += 2)
                putSize += req->arguments.at(i + 1).toUShort(&ok, 16);
        }
        req->owner->expectedDataSize += putSize;
        sDebug() << "Adding a Put command in queue, adding size" << req->owner->expectedDataSize;
    }
}

void WSServer::onBinaryDataReceivedFromClient(const QByteArray& data)
{
    AClient* client = qobject_cast<AClient*>(sender());
    ADevice* dev = client->attachedTo;
    sDebug() << client->name << "Received binary data" << data.size() << "Expected size: " << client->expectedDataSize;
    if (client->commandState != AClient::ClientCommandState::WAITINGBDATAREPLY && client->expectedDataSize == 0)
    {
        client->sendError(ErrorType::ProtocolError, "Sending binary data when nothing waiting for it");
        return;
    }
    // IPS stuff, please don't queue IPS data ~~
    if (client->ipsSize != 0)
    {
        sDebug() << "Data sent are IPS data";
        client->ipsData.append(data);
        if (client->ipsData.size() == client->ipsSize)
        {
            client->recvData.clear();
            client->byteReceived = 0;
            client->ipsSize = 0;
            processIpsData(client);
        }
        return ;
    }
    sDebug() << "Current put size : " << client->currentPutSize << "Expected size" << client->expectedDataSize;
    // There is probably code that can be merged, but it's easier to read
    // to clearly separate all case, if only C++ has local function x)

    // First case, a put command wait for its data and there is no other put queued
    if (client->currentPutSize == client->expectedDataSize) {
        /*client->byteReceived = 0;
        client->recvData.clear();*/
        /*if (data.size() == client->currentPutSize)
        {
            client->currentPutSize = 0; // the call to write data can trigger other signal.
            client->expectedDataSize = 0;
            dev->writeData(data);
            return ;
        }*/
        sDebug() << "Regular non queued put command";
        if (data.size() <= client->currentPutSize) // We need two case since the previous one can
        // trigger the finish signal and mess up currentPutSize
        {
            client->currentPutSize -= data.size();
            client->expectedDataSize = client->currentPutSize;
            dev->writeData(data);
        } else { // There is too much data
            dev->writeData(data.left(client->currentPutSize));
            client->sendError(ErrorType::ProtocolError, "Sending too much binary data");
        }
        return ;
    }
    // If we are here, they are pending put requests.
    // Remember that writeData can trigger finished on some device
    // And that triggers the processing of queued commands.
    // So we need to set variable before otherwise they are overwritted

    // No extra data for the queued request
    if (data.size() <= client->currentPutSize)
    {
        client->currentPutSize -= data.size();
        client->expectedDataSize -= data.size();
        dev->writeData(data);
        return ;
    }

    // We need to put data for futur request in queue before
    // In case writedata trigger finished


    client->recvData.append(data.mid(client->currentPutSize));
    client->byteReceived += data.mid(client->currentPutSize).size();
    unsigned int currentSize = client->currentPutSize;
    client->currentPutSize = 0;
    client->expectedDataSize -= currentSize;
    dev->writeData(data.left(currentSize));
    return ;
}

void WSServer::onClientDisconnected()
{
    AClient* client = qobject_cast<AClient*>(sender());
    sInfo() << "Client disconnected" << client->name;
    cleanUpClient(client);
}

// FIXME
void WSServer::onClientError()
{
    AClient* client = qobject_cast<AClient*>(sender());
    sInfo() << "Client error for client " << client->name;
    //client->errorOccured()
}

void WSServer::onDeviceCommandFinished()
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos[device].currentClient != nullptr)
    {
        processDeviceCommandFinished(device);
        // This should avoid too much recursion
        //QMetaObject::invokeMethod(this, "processCommandQueue", Qt::QueuedConnection, Q_ARG(ADevice*, device));
        processCommandQueue(device);
    }
    else
        sDebug() << "Received finished command while no socket to receive it";
}

void WSServer::onDeviceProtocolError()
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    sInfo() << "Device Error" << device->name();
    setError(ErrorType::ProtocolError, "Error in device protocol");
    disconnect(device, &ADevice::closed, this, &WSServer::onDeviceClosed);
    cleanUpDevice(device);
}

void WSServer::onDeviceClosed()
{
    ADevice* dev = qobject_cast<ADevice*>(sender());
    sInfo() << "Device closed" << dev->name();
    disconnect(dev, &ADevice::closed, this, &WSServer::onDeviceClosed);
    cleanUpDevice(dev);
}

void WSServer::onDeviceGetDataReceived(QByteArray data)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    if (devicesInfos.value(device).currentClient == nullptr)
    {
        sDebug() << "NOOP Sending get data to nothing" << device->name();
        return ;
    }
    sDebug() << "Sending " << data.size() << "to" << devicesInfos[device].currentClient->name;
    devicesInfos[device].currentClient->sendData(data);
}

// Used for Get File
void WSServer::onDeviceSizeGet(unsigned int size)
{
    ADevice*  device = qobject_cast<ADevice*>(sender());
    sendReply(devicesInfos[device].currentClient, QString::number(size, 16));
}

void        WSServer::processCommandQueue(ADevice* device)
{
    QList<MRequest*>&    cmdQueue = pendingRequests[device];
    if (!cmdQueue.isEmpty())
    {
        sDebug() << cmdQueue.size() << " requests in queue, processing the first";
        MRequest* req = cmdQueue.takeFirst();
        currentRequests[device] = req;
        devicesInfos[device].currentCommand = req->opcode;
        devicesInfos[device].currentClient = req->owner;
        req->wasPending = true;
        // Request is no longer in queue, so expected data need to not go in queue
        // if not already here.
        /*if (req->opcode == USB2SnesWS::PutAddress)
        {
            WSInfos& wInfos = wsInfos[req->owner];
            if (wInfos.pendingPutReqWithNoData.contains(req))
            {
                wInfos.pendingPutReqWithNoData.removeFirst();
                wInfos.pendingPutSizes.removeFirst();
            }
        }*/
        executeRequest(req);
    }
}

void WSServer::setError(const Core::ErrorType type, const QString reason)
{
    m_errorType = type;
    m_errorString = reason;
}

void    WSServer::cleanUpDevice(ADevice* device)
{
    sDebug() << "Cleaning up device " << device->name();
    DeviceInfos& devInfo = devicesInfos[device];
    QList<AClient*> toDiscard;
    for (AClient* client : clients)
    {
        if (client->attachedTo == device)
            toDiscard.append(client);
    }
    for (AClient* client : toDiscard)
    {
        client->sendError(m_errorType, m_errorString);
        client->attached = false;
    }
    if (devInfo.currentClient != nullptr)
        devInfo.currentClient = nullptr;
    pendingRequests.remove(device);
    DeviceFactory* devFact = mapDevFact[device];
    mapDevFact.remove(device);
    disconnect(device, nullptr, this, nullptr);
    devices.removeAll(device);
    devFact->deleteDevice(device);
}

// FIXME
void WSServer::cleanUpClient(AClient* client)
{
    sDebug() << "Cleaning up client" << client->name;
    if (pendingDeviceListClients.contains(client))
    {
        pendingDeviceListQuery -= pendingDeviceListClients.count(client);
        pendingDeviceListClients.removeAll(client);
        QMutableListIterator<MRequest> it(pendingDeviceListRequests);
        while (it.hasNext())
        {
            it.next();
            MRequest& req = it.value();
            if (req.owner == client)
            {
                req.owner = nullptr;
                req.state = RequestState::CANCELLED;
                it.remove();
            }
        }
    }
    if (client->attached)
    {
        ADevice*    dev = client->attachedTo;
        MRequest*   req = currentRequests[dev];
        if (devicesInfos.value(dev).currentClient == client)
            devicesInfos[dev].currentClient = nullptr;
        if (req != nullptr && req->owner == client)
        {
            req->owner = nullptr;
            req->state = RequestState::CANCELLED;
        }
        // Removing pending request that are tied to this ws
        QMutableListIterator<MRequest*>    it(pendingRequests[dev]);
        while(it.hasNext())
        {
            MRequest* mReq = it.next();
            if (mReq->owner == client)
            {
                it.remove();
                delete mReq;
            }
        }
    }
    qobject_cast<AClientProvider*>(client->parent())->deleteClient(client);
}

bool WSServer::isValidUnAttached(const USB2SnesWS::opcode opcode)
{
    if (opcode == USB2SnesWS::Attach ||
        opcode == USB2SnesWS::AppVersion ||
        opcode == USB2SnesWS::Name ||
        opcode == USB2SnesWS::DeviceList ||
        opcode == USB2SnesWS::Close)
        return true;
    return false;
}

void        WSServer::sendReply(AClient* client, const QStringList& args)
{
    if (client == nullptr)
    {
        sDebug() << "NOOP: Sending reply to a non existing client";
        return ;
    }
    client->sendReply(args);
}

void    WSServer::sendReply(AClient *ws, QString args)
{
    sendReply(ws, QStringList() << args);
}

bool    WSServer::isV2WebSocket(QWebSocket *ws)
{
    return false;
}


QDebug Core::operator<<(QDebug debug, const MRequest &req)
{
    debug << req.id << "Created at" << req.timeCreated << "-" << req.opcode << req.space << req.flags << req.arguments << req.state;
    return debug;
}
