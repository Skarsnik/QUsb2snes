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

#include "devices/remoteusb2sneswdevice.h"
#include "utils/ipsparse.h"
#include "wsserver.h"
#include <QLoggingCategory>
#include <QSerialPortInfo>
#ifndef QUSB2SNES_NOGUI
  #include <QApplication>
#else
  #include <QCoreApplication>
#endif


bool    WSServer::isFileCommand(USB2SnesWS::opcode opcode)
{
    return (opcode == USB2SnesWS::GetFile || opcode == USB2SnesWS::PutFile || opcode == USB2SnesWS::List ||
            opcode == USB2SnesWS::Rename || opcode == USB2SnesWS::MakeDir || opcode == USB2SnesWS::Rename ||
            opcode == USB2SnesWS::Remove);
}

bool    WSServer::isControlCommand(USB2SnesWS::opcode opcode)
{
    return (opcode == USB2SnesWS::Boot || opcode == USB2SnesWS::Reset || opcode == USB2SnesWS::Menu);
}

#define sDebug() qCDebug(log_wsserver)
#define sInfo() qCInfo(log_wsserver)

#define CMD_TAKE_ONE_ARG(cmd) if (req->arguments.size() != 1) \
{ \
    client->sendError(ErrorType::CommandError, QString("%1 command take one argument").arg(cmd)); \
    return ; \
}


void WSServer::executeServerRequest(MRequest* req)
{
    AClient*    client = req->owner;
    client->currentOpcode = req->opcode;
    sInfo() << "Executing server request : " << *req << "for" << client->name;
    switch (req->opcode)
    {
    case USB2SnesWS::DeviceList : {
        if (numberOfAsyncFactory == 0) {
            QStringList l = getDevicesList();
            sInfo() << "Server request " << *req << " executed in " << req->timeCreated.msecsTo(QTime::currentTime());
            sendReply(client, l);
        } else {
            pendingDeviceListClients.append(client);
            pendingDeviceListRequests.append(*req);
            if (pendingDeviceListClients.size() == 1)
                asyncDeviceList();
        }
        break;
    }
    case USB2SnesWS::Attach : {
        CMD_TAKE_ONE_ARG("Attach")
        cmdAttach(req);
        break;
    }
    case USB2SnesWS::AppVersion : {
        if (client->legacy)
            sendReply(client, "7.42.0");
        else
            sendReply(client, "QUsb2Snes-" + qApp->applicationVersion());
        break;
    }
    case USB2SnesWS::Name : {
        CMD_TAKE_ONE_ARG("Name")
        client->name = req->arguments.at(0);
        break;
    }
    case USB2SnesWS::Close : {
        client->close();
        cleanUpClient(client);
        break;
    }
    default:
        break;
    }
    if (req->opcode != USB2SnesWS::DeviceList)
    {
        sInfo() << "Server request " << *req << " executed in " << req->timeCreated.msecsTo(QTime::currentTime());
    }
    delete req;
}

void    WSServer::executeRequest(MRequest *req)
{
    ADevice*  device = nullptr;
    AClient* client = req->owner;
    client->currentOpcode = req->opcode;
    if (isValidUnAttached(req->opcode))
    {
        executeServerRequest(req);
        return;
    }
    device = client->attachedTo;
    sInfo() << "Executing request : " << *req << "for" << client->name;
    if (client->attached)
        device = client->attachedTo;
    switch(req->opcode)
    {
    case USB2SnesWS::Info : {
        device->infoCommand();
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    /*
     * Control commands
     */
    case USB2SnesWS::Reset :
    {
        device->controlCommand(SD2Snes::opcode::RESET);
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::Menu :
    {
        device->controlCommand(SD2Snes::opcode::MENU_RESET);
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::Boot :
    {
        CMD_TAKE_ONE_ARG("Boot")
        device->controlCommand(SD2Snes::opcode::BOOT, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }

    /*
     * File commands
    */

    case USB2SnesWS::List : {
        CMD_TAKE_ONE_ARG("List")
        device->fileCommand(SD2Snes::opcode::LS, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::GetFile : {
        CMD_TAKE_ONE_ARG("GetFile")
        connect(device, &ADevice::getDataReceived, this, &WSServer::onDeviceGetDataReceived, Qt::UniqueConnection);
        connect(device, &ADevice::sizeGet, this, &WSServer::onDeviceSizeGet, Qt::UniqueConnection);
        device->fileCommand(SD2Snes::opcode::GET, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::PutFile : {
        if (req->arguments.size() != 2)
        {
            client->sendError(ErrorType::CommandError, "PutFile command take 2 arguments (file1, SizeInHex)");
            return ;
        }
        bool ok;
        device->putFile(req->arguments.at(0).toLatin1(), req->arguments.at(1).toUInt(&ok, 16));
        req->state = RequestState::WAITINGREPLY;
        client->commandState = AClient::ClientCommandState::WAITINGBDATAREPLY;
        client->currentPutSize = req->arguments.at(1).toUInt(&ok, 16);
        break;
    }
    case USB2SnesWS::Rename : {
        if (req->arguments.size() != 2)
        {
            client->sendError(ErrorType::CommandError, "Rename command take 2 arguments (file1, file2)");
            return ;
        }
        device->fileCommand(SD2Snes::opcode::MV, QVector<QByteArray>() << req->arguments.at(0).toLatin1()
                                                                    << req->arguments.at(1).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::Remove : {
        CMD_TAKE_ONE_ARG("Remove")
        device->fileCommand(SD2Snes::opcode::RM, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::MakeDir : {
        CMD_TAKE_ONE_ARG("MakeDir")
        device->fileCommand(SD2Snes::opcode::MKDIR, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }

    /*
    * Address command
    */
    case USB2SnesWS::GetAddress : {
        if (req->arguments.size() < 2)
        {
            client->sendError(ErrorType::CommandError, "GetAddress commands take at least 2 arguments (AddressInHex, SizeInHex)");
            return ;
        }
        connect(device, &ADevice::getDataReceived, this, &WSServer::onDeviceGetDataReceived, Qt::UniqueConnection);
        //connect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
        bool    ok;
        if (req->arguments.size() == 2)
        {
            if (req->arguments.at(1).toUInt(&ok, 16) == 0)
            {
                client->sendError(ErrorType::CommandError, "GetAddress - trying to read 0 byte");
                return ;
            }
            device->getAddrCommand(req->space, req->arguments.at(0).toUInt(&ok, 16), req->arguments.at(1).toUInt(&ok, 16));
        } else {

            QList<QPair<unsigned int, unsigned int> > pairs;
            // NOTE a size > 255 is ignored by the original server
            // When it should probably be an error, but the ZeldaHub software use it
            // Let fallback of spliting the request when it's a legacy connection
            bool split = false;
            for (int i = 0; i < req->arguments.size(); i += 2)
            {
                unsigned usize = req->arguments.at(i + 1).toUInt(&ok, 16);
                if (usize == 0)
                {
                    client->sendError(ErrorType::CommandError, "GetAddress - trying to read 0 byte");
                    return ;
                }
                if (usize > 255)
                {
                    if (client->legacy)
                    {
                        split = true;
                    } else {
                        client->sendError(ErrorType::CommandError, "GetAddress - VGet with a size > 255");
                        return ;
                    }
                }
                pairs.append(QPair<unsigned int, unsigned int>(req->arguments.at(i).toUInt(&ok, 16), usize));
            }
            if (!split && device->hasVariaditeCommands())
            {
                QList<QPair<unsigned int, quint8>> translatedPairs;
                for (auto mpair : pairs)
                {
                    translatedPairs.append(QPair<unsigned int, quint8>(mpair.first, static_cast<quint8>(mpair.second)));
                }
                device->getAddrCommand(req->space, translatedPairs);
            } else {
                device->getAddrCommand(req->space, req->arguments.at(0).toUInt(&ok, 16), req->arguments.at(1).toUInt(&ok, 16));
                for (int i = 1; i < pairs.size(); i++)
                {
                    sDebug() << pairs.at(i);
                    MRequest* newReq = new MRequest();
                    newReq->owner = client;
                    newReq->state = RequestState::NEW;
                    newReq->space = SD2Snes::space::SNES;
                    newReq->timeCreated = QTime::currentTime();
                    newReq->wasPending = false;
                    newReq->opcode = USB2SnesWS::GetAddress;
                    newReq->arguments.append(QString::number(pairs.at(i).first, 16));
                    newReq->arguments.append(QString::number(pairs.at(i).second, 16));
                    pendingRequests[device].insert(i - 1, newReq);
                }
            }
        }
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::PutAddress : {
        if (req->arguments.size() < 2)
        {
            client->sendError(ErrorType::CommandError, "PutAddress command take at least 2 arguments (AddressInHex, SizeInHex)");
            return ;
        }
        bool ok;
        unsigned int  putSize = 0;
        bool spliced = false;
        // Basic usage of PutAddress
        // FIXME add flags in VPUT
        if (req->arguments.size() == 2)
        {
            putSize = req->arguments.at(1).toUInt(&ok, 16);
            if (putSize == 0)
            {
                client->sendError(ErrorType::CommandError, "PutAddress - trying to write 0 byte");
                return ;
            }
            if (req->flags.isEmpty())
                device->putAddrCommand(req->space, req->arguments.at(0).toUInt(&ok, 16), req->arguments.at(1).toUInt(&ok, 16));
            else {
                unsigned char flags = 0;
                foreach(QString flag, req->flags)
                {
                    flags |= static_cast<SD2Snes::server_flags>(flagsMetaEnum.keyToValue(qPrintable(flag)));
                }
                device->putAddrCommand(req->space, flags, req->arguments.at(0).toUInt(&ok, 16), req->arguments.at(1).toUInt(&ok, 16));
            }
        } else {
            QList<QPair<unsigned int, quint8> > vputArgs;
            for (int i = 0; i < req->arguments.size(); i += 2)
            {
                if (req->arguments.at(i + 1).toUInt(&ok, 16) == 0)
                {
                    client->sendError(ErrorType::CommandError, "PutAddress - trying to write 0 byte");
                    return ;
                }
                vputArgs.append(QPair<unsigned int, quint8>(req->arguments.at(i).toUInt(&ok, 16), req->arguments.at(i + 1).toUShort(&ok, 16)));
                putSize += req->arguments.at(i + 1).toUShort(&ok, 16);
            }
            if (device->hasVariaditeCommands())
            {
                device->putAddrCommand(req->space, vputArgs);
            } else { // Please don't use VPUT
                sDebug() << "VPUT that get spliced";
                //spliced = true;
                device->putAddrCommand(req->space, vputArgs.at(0).first, vputArgs.at(0).second);
                putSize = vputArgs.at(0).second;
                unsigned int totalSize = putSize;
                // We should probably handle incomplete queued data, but who mad
                // people will not send all bytes at once.
                vputArgs.removeFirst();
                QListIterator<QPair<unsigned int, quint8> > argsIt(vputArgs);
                int cpt = 0;
                while (argsIt.hasNext())
                {
                    auto pair = argsIt.next();
                    MRequest* newReq = new MRequest();
                    newReq->owner = client;
                    newReq->state = RequestState::NEW;
                    newReq->space = SD2Snes::space::SNES;
                    newReq->timeCreated = QTime::currentTime();
                    newReq->wasPending = true;
                    newReq->opcode = USB2SnesWS::PutAddress;
                    newReq->arguments.append(QString::number(pair.first, 16));
                    newReq->arguments.append(QString::number(pair.second, 16));
                    totalSize += pair.second;
                    pendingRequests[device].insert(cpt, newReq);
                    cpt++;
                }
                if (!req->wasPending)
                    client->expectedDataSize = totalSize;
            }
        }
        req->state = RequestState::WAITINGREPLY;
        client->commandState = AClient::ClientCommandState::WAITINGBDATAREPLY;
        //sDebug() << "Writing before cps :" << __func__ << wsInfos[ws].currentPutSize;
        client->currentPutSize = putSize;
        if (client->expectedDataSize == 0)
            client->expectedDataSize = putSize;
        //sDebug() << "Writing after cps" << __func__ << wsInfos[ws].currentPutSize;
        break;
    }

    /*
     * PutIPS
    */
    case USB2SnesWS::PutIPS : {
        if (req->arguments.size() < 2)
        {
            client->sendError(ErrorType::CommandError, "PutIPS command take at least 2 arguments (Name, SizeInHex)");
            return ;
        }
        bool    ok;
        req->state = RequestState::WAITINGREPLY;
        client->commandState = AClient::ClientCommandState::WAITINGBDATAREPLY;
        client->ipsSize = req->arguments.at(1).toUInt(&ok, 16);
        break;
    }
    default:
    {
        client->sendError(ErrorType::ProtocolError, "Invalid command or non implemented");
        return ;
    }
    }

    // There are multiple case with pending put request
    // 1A : this was the only request in queue                   -> send recvData
    // 1B : this was the only request in queue, no data          -> do nothing
    // 1C : this was the only request in queue, missing data     -> send recvData
    // 2A : other pending request, all data are here             -> send recvData
    // 2B : other pending request, no data                       -> do nothing
    // 2C : other pending request, missing data                  -> send recvData
    /*
     *
    */

    if (req->wasPending && (req->opcode == USB2SnesWS::PutFile || req->opcode == USB2SnesWS::PutAddress))
    {
        client->commandState = AClient::ClientCommandState::WAITINGBDATAREPLY;
        if (client->recvData.size() >= client->currentPutSize) // This cover 1A and 2A
        {                                           // Only imcomplete data can be for later requests if this is not empty
            //sDebug() << "Pending 1A & 2A";
            sDebug() << "We have ALL data for the queued command " << client->expectedDataSize;
            QByteArray toSend = client->recvData.left(client->currentPutSize);
            client->recvData.remove(0, client->currentPutSize);
            client->expectedDataSize -= client->currentPutSize;
            client->currentPutSize = 0;
            device->writeData(toSend);
            client->commandState = AClient::ClientCommandState::WAITINGREPLY;

        } else {
            if (!client->recvData.isEmpty() && client->recvData.size() < client->currentPutSize)
            {
                sDebug() << "We have SOME data for the queued command " << client->expectedDataSize;
                device->writeData(client->recvData);
                client->expectedDataSize -= client->recvData.size();
                client->currentPutSize -= client->recvData.size();
                client->recvData.clear();
            } else {
                // Nothing;
            }
        }
    }
    sDebug() << "Request executed";
    return ;
}

#undef CMD_TAKE_ONE_ARG





void    WSServer::processDeviceCommandFinished(ADevice* device)
{
    DeviceInfos&  info = devicesInfos[device];
    sDebug() << "Processing command finished" << info.currentCommand;
    //sDebug() << "Wriging before cps : " << __func__ << wsInfos[info.currentWS].currentPutSize;
    info.currentClient->currentPutSize = 0;
    //sDebug() << "Wriging after cps :" << __func__ << wsInfos[info.currentWS].currentPutSize;
    switch (info.currentCommand) {
    case USB2SnesWS::Info :
    {
        USB2SnesInfo    ifo = device->parseInfo(device->dataRead);
        sendReply(info.currentClient, QStringList() << ifo.version << ifo.deviceName << ifo.romPlaying << ifo.flags);
        break;
    }

    // FILE Command
    case USB2SnesWS::List :
    {
        QList<ADevice::FileInfos> lfi = device->parseLSCommand(device->dataRead);
        QStringList rep;
        foreach(ADevice::FileInfos fi, lfi)
        {
            rep << QString::number(static_cast<quint32>(fi.type));
            rep << fi.name;
        }
        sendReply(info.currentClient, rep);
        break;
    }
    case USB2SnesWS::GetFile :
    {
        disconnect(device, &ADevice::getDataReceived, this, &WSServer::onDeviceGetDataReceived);
        disconnect(device, &ADevice::sizeGet, this, &WSServer::onDeviceSizeGet);
        break;
    }
    case USB2SnesWS::Rename :
    case USB2SnesWS::Remove :
    case USB2SnesWS::MakeDir :
    case USB2SnesWS::PutFile :
    case USB2SnesWS::PutAddress :
    case USB2SnesWS::Menu :
    case USB2SnesWS::Reset :
    case USB2SnesWS::Boot :
    {
        sendReply(info.currentClient, "");
        break;
    }
    case USB2SnesWS::GetAddress :
    {
        disconnect(device, &ADevice::getDataReceived, this, &WSServer::onDeviceGetDataReceived);
        //disconnect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
        break;
    }
    default:
    {
        sDebug() << "Error, command not found";
        return;
    }
    }
    currentRequests[device]->state = RequestState::DONE;
    sInfo() << "Device request finished - " << *(currentRequests.value(device)) << "processed in " << currentRequests.value(device)->timeCreated.msecsTo(QTime::currentTime()) << " ms";
    delete currentRequests[device];
    currentRequests[device] = nullptr;
}

/*
 * Attach stuff
 */

// This will be deprecated when all devicefactory are made async

QStringList WSServer::getDevicesList()
{
    QStringList toret;
    sDebug() << "Device List";
    foreach (DeviceFactory* devFact, deviceFactories)
    {
        toret.append(devFact->listDevices());
    }
    return toret;
}

void    WSServer::asyncDeviceList()
{
    sDebug() << "Async device list";
    deviceList.clear();
    pendingDeviceListQuery = 0;
    for (auto devFact : qAsConst(deviceFactories))
    {
        if (!devFact->hasAsyncListDevices())
        {
            deviceList.append(devFact->listDevices());
        }
    }
    for (auto devFact : qAsConst(deviceFactories))
    {
        if (devFact->hasAsyncListDevices())
        {
            pendingDeviceListQuery++;
        }
    }
    sDebug() << "Doing device list for " << pendingDeviceListQuery << " factories";
    for (auto devFact : qAsConst(deviceFactories))
    {
        if (devFact->hasAsyncListDevices())
        {
            sDebug() << "Async list device for " << devFact->name();
            devFact->asyncListDevices();
        }
    }
}

void    WSServer::onDeviceListDone()
{
    sDebug() << qobject_cast<DeviceFactory*>(sender())->name() << " is done doing devicelist";
    pendingDeviceListQuery--;
    if (pendingDeviceListQuery != 0)
        return;
    for (AClient* client : pendingDeviceListClients)
    {
        sDebug() << "Sending device list to " << client->name;
        sendReply(client, deviceList);
    }
    for (const MRequest& req : pendingDeviceListRequests)
    {
        sInfo() << "Device request finished - " << req << "processed in " << req.timeCreated.msecsTo(QTime::currentTime()) << " ms";
    }
    pendingDeviceListQuery = 0;
    deviceList.clear();
    pendingDeviceListRequests.clear();
    pendingDeviceListClients.clear();
}

void    WSServer::onNewDeviceName(QString name)
{
    sDebug() << "Received a device name : " << name;
    deviceList.append(name);
}

void WSServer::cmdAttach(MRequest *req)
{
    QString deviceToAttach = req->arguments.at(0);
    req->owner->pendingAttach = true;
    ADevice* devGet = nullptr;

    foreach (DeviceFactory* devFact, deviceFactories)
    {
        sDebug() << devFact->name();
        devGet = devFact->attach(deviceToAttach);
        if (devGet != nullptr)
        {
            mapDevFact[devGet] = devFact;
            if (qobject_cast<RemoteUsb2SnesWFactory*>(devFact) != nullptr)
            {
                req->owner->attachedToRemote = true;
                setRemoteConnection(nullptr, req->owner, devGet);
                // Since we don't call open, there should be not pending command happening.
                req->owner->pendingAttach = false;
                return;
            }
            break;
        }
        if (devFact->attachError().isEmpty() == false)
        {
            req->owner->sendError(ErrorType::CommandError, "Attach Error with " + deviceToAttach + " - " + devFact->attachError());
            return ;
        }
    }
    if (devGet != nullptr)
    {
        sDebug() << "Found device" << devGet->name() << "from" << mapDevFact[devGet]->name() << "State : " << devGet->state();
        sDebug() << "Attaching " << req->owner->name <<  " to " << deviceToAttach;
        if (devGet->state() == ADevice::State::CLOSED)
        {
            sDebug() << "Trying to open device";
            if (devGet->open() == false)
            {
                req->owner->sendError(ErrorType::CommandError, "Attach: Can't open the device on " + deviceToAttach);
                return;
            }
        }
        if (!devices.contains(devGet))
            addDevice(devGet);
        req->owner->attached = true;
        req->owner->attachedTo = devGet;
        req->owner->pendingAttach = false;
        sendReply(req->owner, devGet->name());
        if (devGet->state() == ADevice::READY)
            processCommandQueue(devGet);
        return ;
    } else {
        req->owner->sendError(ErrorType::CommandError, "Trying to Attach to an unknow device");
        return ;
    }
}

/* This set the connection to the remote, the main code stop caring about the
 * the communication at this point, only the device will log trafic.
 */
void    WSServer::setRemoteConnection(QWebSocket* ws, AClient* infos, ADevice* device)
{
    sDebug() << "Attaching to remote server";
    infos->attachedTo = device;
    RemoteUsb2snesWDevice* remoteDevice = qobject_cast<RemoteUsb2snesWDevice*>(device);
    remoteDevice->setClientName(infos->name);
    connect(remoteDevice, &RemoteUsb2snesWDevice::textMessageReceived, ws, [=](const QString& message)
    {
        ws->sendTextMessage(message);
    });
    connect(remoteDevice, &RemoteUsb2snesWDevice::binaryMessageReceived, ws, [=](const QByteArray& message)
    {
        ws->sendBinaryMessage(message);
    });
    //disconnect(ws, &QWebSocket::textMessageReceived, this, &WSServer::onTextMessageReceived);
    disconnect(ws, &QWebSocket::binaryMessageReceived, this, &WSServer::onBinaryMessageReceived);
    connect(ws, &QWebSocket::textMessageReceived, remoteDevice, &RemoteUsb2snesWDevice::sendText);
    connect(ws, &QWebSocket::binaryMessageReceived, remoteDevice, &RemoteUsb2snesWDevice::sendBinary);
}

void    WSServer::processIpsData(AClient *client)
{
    sDebug() << "processing IPS data";
    MRequest* ipsReq = currentRequests[client->attachedTo];
    QList<IPSReccord>  ipsReccords = parseIPSData(client->ipsData);

    // Creating new PutAddress requests
    int cpt = 0;
    sDebug() << "Creating " << ipsReccords.size() << "PutRequests";
    foreach(IPSReccord ipsr, ipsReccords)
    {
        MRequest* newReq = new MRequest();
        newReq->owner = client;
        newReq->state = RequestState::NEW;
        newReq->space = SD2Snes::space::SNES;
        newReq->timeCreated = QTime::currentTime();
        newReq->wasPending = false;
        newReq->opcode = USB2SnesWS::PutAddress;
        // Special stuff for patch that install stuff in the NMI of sd2snes
        if (cpt == 0 && ipsReq->arguments.at(0) == "hook")
            newReq->flags << "CLRX";
        if (cpt == (ipsReccords.size() - 1) && ipsReq->arguments.at(0) == "hook")
            newReq->flags << "SETX";
        newReq->arguments << QString::number(ipsr.offset, 16) << QString::number(ipsr.size, 16);

        pendingRequests[client->attachedTo].append(newReq);
        client->recvData.append(ipsr.data);
        //sDebug() << "IPS:" <<  QString::number(ipsr.offset, 16) << ipsr.data.toHex();
        cpt++;
    }
    client->ipsData.clear();
    processCommandQueue(client->attachedTo);
}
