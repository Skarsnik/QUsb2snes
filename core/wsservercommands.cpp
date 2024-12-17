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

#include "ipsparse.h"
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
    setError(ErrorType::CommandError, QString("%1 command take one argument").arg(cmd)); \
    clientError(ws); \
    return ; \
}

// TODO, need to delete req when async device list is done

void WSServer::executeServerRequest(MRequest* req)
{
    QWebSocket*     ws = req->owner;
    sInfo() << "Executing server request : " << *req << "for" << wsInfos.value(ws).name;
    switch (req->opcode)
    {
    case USB2SnesWS::DeviceList : {
        if (numberOfAsyncFactory == 0) {
            QStringList l = getDevicesList();
            sendReply(ws, l);
        } else {
            pendingDeviceListWebsocket.append(ws);
            pendingDeviceListRequests.append(req);
            if (pendingDeviceListWebsocket.size() == 1)
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
        if (wsInfos.value(ws).legacy)
            sendReply(ws, "7.42.0");
        else
            sendReply(ws, "QUsb2Snes-" + qApp->applicationVersion());
        break;
    }
    case USB2SnesWS::Name : {
        CMD_TAKE_ONE_ARG("Name")
        wsInfos[ws].name = req->arguments.at(0);
        sendReplyV2(ws, wsInfos[ws].name);
        break;
    }
    case USB2SnesWS::Close : {
        ws->close();
        cleanUpSocket(ws);
        break;
    }
    default:
        break;
    }
    if (req->opcode != USB2SnesWS::DeviceList)
    {
        sInfo() << "Server request " << *req << " executed in " << req->timeCreated.msecsTo(QTime::currentTime());
        delete req;
    }
}

void    WSServer::executeRequest(MRequest *req)
{
    ADevice*  device = nullptr;
    QWebSocket*     ws = req->owner;
    if (isValidUnAttached(req->opcode))
    {
        executeServerRequest(req);
        return;
    }
    device = wsInfos.value(ws).attachedTo;
    sInfo() << "Executing request : " << *req << "for" << wsInfos.value(ws).name;
    if (wsInfos.value(ws).attached)
        device = wsInfos.value(ws).attachedTo;
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
            setError(ErrorType::CommandError, "PutFile command take 2 arguments (file1, SizeInHex)");
            clientError(ws);
            return ;
        }
        bool ok;
        device->putFile(req->arguments.at(0).toLatin1(), req->arguments.at(1).toUInt(&ok, 16));
        req->state = RequestState::WAITINGREPLY;
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
        wsInfos[ws].currentPutSize = req->arguments.at(1).toUInt(&ok, 16);
        break;
    }
    case USB2SnesWS::Rename : {
        if (req->arguments.size() != 2)
        {
            setError(ErrorType::CommandError, "Rename command take 2 arguments (file1, file2)");
            clientError(ws);
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
            setError(ErrorType::CommandError, "GetAddress commands take at least 2 arguments (AddressInHex, SizeInHex)");
            clientError(ws);
            return ;
        }
        connect(device, &ADevice::getDataReceived, this, &WSServer::onDeviceGetDataReceived, Qt::UniqueConnection);
        //connect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
        bool    ok;
        if (req->arguments.size() == 2)
        {
            if (req->arguments.at(1).toUInt(&ok, 16) == 0)
            {
                setError(ErrorType::CommandError, "GetAddress - trying to read 0 byte");
                clientError(ws);
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
                    setError(ErrorType::CommandError, "GetAddress - trying to read 0 byte");
                    clientError(ws);
                    return ;
                }
                if (usize > 255)
                {
                    if (wsInfos.value(ws).legacy)
                    {
                        split = true;
                    } else {
                        setError(ErrorType::CommandError, "GetAddress - VGet with a size > 255");
                        clientError(ws);
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
                    newReq->owner = ws;
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
            setError(ErrorType::CommandError, "PutAddress command take at least 2 arguments (AddressInHex, SizeInHex)");
            clientError(ws);
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
                setError(ErrorType::CommandError, "PutAddress - trying to write 0 byte");
                clientError(ws);
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
                    setError(ErrorType::CommandError, "PutAddress - trying to write 0 byte");
                    clientError(ws);
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
                    newReq->owner = ws;
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
                    wsInfos[ws].expectedDataSize = totalSize;
            }
        }
        req->state = RequestState::WAITINGREPLY;
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
        //sDebug() << "Writing before cps :" << __func__ << wsInfos[ws].currentPutSize;
        wsInfos[ws].currentPutSize = putSize;
        if (wsInfos[ws].expectedDataSize == 0)
            wsInfos[ws].expectedDataSize = putSize;
        //sDebug() << "Writing after cps" << __func__ << wsInfos[ws].currentPutSize;
        break;
    }

    /*
     * PutIPS
    */
    case USB2SnesWS::PutIPS : {
        if (req->arguments.size() < 2)
        {
            setError(ErrorType::CommandError, "PutIPS command take at least 2 arguments (Name, SizeInHex)");
            clientError(ws);
            return ;
        }
        bool    ok;
        req->state = RequestState::WAITINGREPLY;
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
        wsInfos[ws].ipsSize = req->arguments.at(1).toUInt(&ok, 16);
        break;
    }
    default:
    {
        setError(ErrorType::ProtocolError, "Invalid command or non implemented");
        clientError(ws);
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
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
        if (wsInfos[ws].recvData.size() >= wsInfos[ws].currentPutSize) // This cover 1A and 2A
        {                                           // Only imcomplete data can be for later requests if this is not empty
            //sDebug() << "Pending 1A & 2A";
            sDebug() << "We have ALL data for the queued command " << wsInfos[ws].expectedDataSize;
            QByteArray toSend = wsInfos[ws].recvData.left(wsInfos[ws].currentPutSize);
            wsInfos[ws].recvData.remove(0, wsInfos[ws].currentPutSize);
            wsInfos[ws].expectedDataSize -= wsInfos[ws].currentPutSize;
            wsInfos[ws].currentPutSize = 0;
            device->writeData(toSend);
            wsInfos[ws].commandState = ClientCommandState::WAITINGREPLY;

        } else {
            if (!wsInfos[ws].recvData.isEmpty() && wsInfos[ws].recvData.size() < wsInfos[ws].currentPutSize)
            {
                sDebug() << "We have SOME data for the queued command " << wsInfos[ws].expectedDataSize;
                device->writeData(wsInfos[ws].recvData);
                wsInfos[ws].expectedDataSize -= wsInfos[ws].recvData.size();
                wsInfos[ws].currentPutSize -= wsInfos[ws].recvData.size();
                wsInfos[ws].recvData.clear();
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
    wsInfos[info.currentWS].currentPutSize = 0;
    //sDebug() << "Wriging after cps :" << __func__ << wsInfos[info.currentWS].currentPutSize;
    switch (info.currentCommand) {
    case USB2SnesWS::Info :
    {
        USB2SnesInfo    ifo = device->parseInfo(device->dataRead);
        sendReply(info.currentWS, QStringList() << ifo.version << ifo.deviceName << ifo.romPlaying << ifo.flags);
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
        sendReply(info.currentWS, rep);
        break;
    }
    case USB2SnesWS::GetFile :
    {
        disconnect(device, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onDeviceGetDataReceived(QByteArray)));
        disconnect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
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
        sendReplyV2(info.currentWS, "");
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
            if (devFact->asyncListDevices())
                pendingDeviceListQuery++;
        }
    }
}

void    WSServer::onDeviceListDone()
{
    sDebug() << qobject_cast<DeviceFactory*>(sender())->name() << " is done doing devicelist";
    pendingDeviceListQuery--;
    if (pendingDeviceListQuery != 0)
            return;
    for (auto ws : qAsConst(pendingDeviceListWebsocket))
    {
        sDebug() << "Sending device list to " << wsInfos[ws].name;
        sendReply(ws, deviceList);
    }
    for (MRequest* req : qAsConst(pendingDeviceListRequests))
    {
        sInfo() << "Device request finished - " << *req << "processed in " << req->timeCreated.msecsTo(QTime::currentTime()) << " ms";
        delete req;
    }
    pendingDeviceListQuery = 0;
    deviceList.clear();
    pendingDeviceListRequests.clear();
    pendingDeviceListWebsocket.clear();
}

void    WSServer::onNewDeviceName(QString name)
{
    sDebug() << "Received a device name : " << name;
    deviceList.append(name);
}

void WSServer::cmdAttach(MRequest *req)
{
    QString deviceToAttach = req->arguments.at(0);
    wsInfos[req->owner].pendingAttach = true;
    ADevice* devGet = nullptr;

    foreach (DeviceFactory* devFact, deviceFactories)
    {
        sDebug() << devFact->name();
        devGet = devFact->attach(deviceToAttach);
        if (devGet != nullptr)
        {
            mapDevFact[devGet] = devFact;
            break;
        }
        if (!devFact->attachError().isEmpty())
        {
            setError(ErrorType::CommandError, "Attach Error with " + deviceToAttach + " - " + devFact->attachError());
            clientError(req->owner);
            return ;
        }
    }
    if (devGet != nullptr)
    {
        sDebug() << "Found device" << devGet->name() << "from" << mapDevFact[devGet]->name() << "State : " << devGet->state();
        sDebug() << "Attaching " << wsInfos.value(req->owner).name <<  " to " << deviceToAttach;
        if (devGet->state() == ADevice::State::CLOSED)
        {
            sDebug() << "Trying to open device";
            if (!devGet->open())
            {
                setError(ErrorType::CommandError, "Attach: Can't open the device on " + deviceToAttach);
                clientError(req->owner);
                return;
            }
        }
        if (!devices.contains(devGet))
            addDevice(devGet);
        wsInfos[req->owner].attached = true;
        wsInfos[req->owner].attachedTo = devGet;
        wsInfos[req->owner].pendingAttach = false;
        sendReplyV2(req->owner, devGet->name());
        if (devGet->state() == ADevice::READY)
            processCommandQueue(devGet);
        return ;
    } else {
        setError(ErrorType::CommandError, "Trying to Attach to an unknow device");
        clientError(req->owner);
        return ;
    }
}

void    WSServer::processIpsData(QWebSocket* ws)
{
    sDebug() << "processing IPS data";
    WSInfos& infos = wsInfos[ws];
    MRequest* ipsReq = currentRequests[infos.attachedTo];
    QList<IPSReccord>  ipsReccords = parseIPSData(infos.ipsData);

    // Creating new PutAddress requests
    int cpt = 0;
    sDebug() << "Creating " << ipsReccords.size() << "PutRequests";
    foreach(IPSReccord ipsr, ipsReccords)
    {
        MRequest* newReq = new MRequest();
        newReq->owner = ws;
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

        pendingRequests[infos.attachedTo].append(newReq);
        infos.recvData.append(ipsr.data);
        //sDebug() << "IPS:" <<  QString::number(ipsr.offset, 16) << ipsr.data.toHex();
        cpt++;
    }
    infos.ipsData.clear();
    processCommandQueue(infos.attachedTo);
}
