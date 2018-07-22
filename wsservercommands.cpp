#include "usbconnection.h"
#include "wsserver.h"
#include <QLoggingCategory>
#include <QSerialPortInfo>

bool    WSServer::isFileCommand(USB2SnesWS::opcode opcode)
{
    return (opcode == USB2SnesWS::GetFile | opcode == USB2SnesWS::PutFile | opcode == USB2SnesWS::List |
            opcode == USB2SnesWS::Rename | opcode == USB2SnesWS::MakeDir | opcode == USB2SnesWS::Rename |
            opcode == USB2SnesWS::Remove);
}

bool    WSServer::isControlCommand(USB2SnesWS::opcode opcode)
{
    return (opcode == USB2SnesWS::Boot | opcode == USB2SnesWS::Reset | opcode == USB2SnesWS::Menu);
}

#define sDebug() qCDebug(log_wsserver)

#define CMD_TAKE_ONE_ARG(cmd) if (req->arguments.size() != 1) \
{ \
    setError(ErrorType::CommandError, QString("%1 command take one argument").arg(cmd)); \
    clientError(ws); \
    return ; \
}

void    WSServer::executeRequest(MRequest *req)
{
    ADevice*  device = NULL;
    QWebSocket*     ws = req->owner;
    sDebug() << "Executing request : " << *req << "for" << wsNames.value(ws);
    if (wsInfos.value(ws).attached)
        device = wsInfos.value(ws).attachedTo;
    switch (req->opcode)
    {
    case USB2SnesWS::DeviceList : {
        QStringList l = getDevicesList();
        sendReply(ws, l);
        break;
    }
    case USB2SnesWS::Attach : {
        CMD_TAKE_ONE_ARG("Attach")
        cmdAttach(req);
        break;
    }
    case USB2SnesWS::AppVersion : {
        sendReply(ws, "7.42.0");
        break;
    }
    case USB2SnesWS::Name : {
        CMD_TAKE_ONE_ARG("Name")
        wsNames[ws] = req->arguments.at(0);
        break;
    }
    case USB2SnesWS::Info : {
        device->infoCommand();
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
        connect(device, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onDeviceGetDataReceived(QByteArray)));
        connect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
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
        device->putFile(req->arguments.at(0).toLatin1(), req->arguments.at(1).toInt(&ok, 16));
        req->state = RequestState::WAITINGREPLY;
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
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
        if (req->arguments.size() != 2)
        {
            setError(ErrorType::CommandError, "GetAddress command take 2 arguments (AddressInHex, SizeInHex)");
            clientError(ws);
            return ;
        }
        connect(device, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onDeviceGetDataReceived(QByteArray)));
        connect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
        bool    ok;
        device->getAddrCommand(req->space, req->arguments.at(0).toInt(&ok, 16), req->arguments.at(1).toInt(&ok, 16));
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
        // Basic usage of PutAddress
        if (req->arguments.size() == 2)
        {
            device->putAddrCommand(req->space, req->arguments.at(0).toInt(&ok, 16), req->arguments.at(1).toInt(&ok, 16));
            if (req->wasPending)
            {
                device->writeData(wsInfos[ws].pendingPutDatas.takeFirst());
            }
        } else {
           QList<QPair<unsigned int, quint8> > pairs;
           for (unsigned int i = 0; i < req->arguments.size(); i += 2)
           {
               pairs.append(QPair<unsigned int, quint8>(req->arguments.at(i).toUInt(&ok, 16), req->arguments.at(i + 1).toUShort(&ok, 16)));
           }
           device->putAddrCommand(req->space, pairs);
        }
        req->state = RequestState::WAITINGREPLY;
        wsInfos[ws].commandState = ClientCommandState::WAITINGBDATAREPLY;
        break;
    }

    default:
    {
        setError(ErrorType::ProtocolError, "Invalid command or non implemented");
        clientError(ws);
    }
    }
    sDebug() << "Request executed";
}

#undef CMD_TAKE_ONE_ARG





void    WSServer::processDeviceCommandFinished(ADevice* device)
{
    DeviceInfos&  info = devicesInfos[device];
    sDebug() << "Processing command finished" << info.currentCommand;
    switch (info.currentCommand) {
    case USB2SnesWS::Info :
    {
        USB2SnesInfo    ifo = device->parseInfo(device->dataRead);
        sendReply(info.currentWS, QStringList() << ifo.version << "foo" << ifo.romPlaying << ifo.flags);
        break;
    }

    // FILE Command
    case USB2SnesWS::List :
    {
        QList<ADevice::FileInfos> lfi = device->parseLSCommand(device->dataRead);
        QStringList rep;
        foreach(ADevice::FileInfos fi, lfi)
        {
            rep << QString::number((quint32) fi.type);
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
    {
        break;
    }
    case USB2SnesWS::GetAddress :
    {
        disconnect(device, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onDeviceGetDataReceived(QByteArray)));
        disconnect(device, SIGNAL(sizeGet(uint)), this, SLOT(onDeviceSizeGet(uint)));
        break;
    }
    default:
    {
        sDebug() << "Error, command not found";
        return;
    }
    }
    currentRequests[device]->state = RequestState::DONE;
    sDebug() << "Request " << *(currentRequests.value(device)) << "processed in " << currentRequests.value(device)->timeCreated.msecsTo(QTime::currentTime()) << " ms";
    delete currentRequests[device];
    currentRequests[device] = NULL;
}


QStringList WSServer::getDevicesList()
{
    QStringList toret;
    sDebug() << "Device List";
    foreach (ADevice* dev, devices)
        toret << dev->name();
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos) {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber() << "Busy : " << usbinfo.isBusy();
        if (usbinfo.isBusy() == false && !toret.contains(usbinfo.portName()) && usbinfo.serialNumber() == "DEMO00000000")
            toret << "SD2SNES " + usbinfo.portName();
    }
    return toret;
}

void WSServer::cmdAttach(MRequest *req)
{
    QString port = req->arguments.at(0);
    bool    portFound = false;
    foreach (ADevice* bla, devices) {
        if (bla->name() == port)
        {
            portFound = true;
            sDebug() << "Found device" << bla->name() << "State : " << bla->state();
            if (bla->state() == ADevice::State::CLOSED)
            {
                sDebug() << "PIJKOOO";
                if (!bla->open())
                {
                    setError(ErrorType::CommandError, "Attach: Can't open the device on " + port);
                    clientError(req->owner);
                    return;
                }
            }
            break;
        }
    }
    if (!portFound)
    {
        QString nPort = port;
        nPort.replace("SD2SNES ", "");
        ADevice* newDev = new USBConnection(nPort);
        if (!newDev->open())
        {
            setError(ErrorType::CommandError, "Attach: Can't open the device on " + port);
            clientError(req->owner);
            return ;
        } else {
            sDebug() << "Added a new low Connection" << port;
            addDevice(newDev);
        }
    }
    foreach (ADevice* dev, devices)
    {
        if (dev->name() == port)
        {
            sDebug() << "Attaching " << wsNames.value(req->owner) <<  " to " << port;
            wsInfos[req->owner].attached = true;
            wsInfos[req->owner].attachedTo = dev;
            return ;
        }
    }
    setError(ErrorType::CommandError, "Trying to Attach to an unknow device");
    clientError(req->owner);
    return ;
}
