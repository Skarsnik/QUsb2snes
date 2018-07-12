#include "wsserver.h"
#include <QLoggingCategory>
#include <QSerialPortInfo>

#define sDebug() qCDebug(log_wsserver)

#define CMD_TAKE_ONE_ARG(cmd) if (req->arguments.size() != 1) \
{ \
    setError(ErrorType::CommandError, QString("%1 command take one argument").arg(cmd)); \
    clientError(ws); \
    return ; \
}

void    WSServer::executeRequest(MRequest *req)
{
    USBConnection*  usbCo = NULL;
    QWebSocket*     ws = req->owner;
    sDebug() << "Executing request : " << *req << "for" << wsNames.value(ws);
    if (wsInfos.value(ws).attached)
        usbCo = wsInfos.value(ws).attachedTo;
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
        usbCo->infoCommand();
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    // FILE Commands
    case USB2SnesWS::List : {
        CMD_TAKE_ONE_ARG("List")
        usbCo->fileCommand(SD2Snes::opcode::LS, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::GetFile : {
        CMD_TAKE_ONE_ARG("GetFile")
        connect(usbCo, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onLowCoGetDataReceived(QByteArray)));
        connect(usbCo, SIGNAL(sizeGet(uint)), this, SLOT(onLowCoSizeGet(uint)));
        usbCo->fileCommand(SD2Snes::opcode::GET, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::GetAddress : {
        if (req->arguments.size() != 2)
        {
            setError(ErrorType::CommandError, "GetAddress command take 2 arguments (AddressInHex, SizeInHex)");
            clientError(ws);
            return ;
        }
        connect(usbCo, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onLowCoGetDataReceived(QByteArray)));
        connect(usbCo, SIGNAL(sizeGet(uint)), this, SLOT(onLowCoSizeGet(uint)));
        bool    ok;
        usbCo->getAddrCommand(req->space, req->arguments.at(0).toInt(&ok, 16), req->arguments.at(1).toInt(&ok, 16));
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
        usbCo->putFile(req->arguments.at(0).toLatin1(), req->arguments.at(1).toInt(&ok, 16));
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
        usbCo->fileCommand(SD2Snes::opcode::MV, QVector<QByteArray>() << req->arguments.at(0).toLatin1()
                                                                    << req->arguments.at(1).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::Remove : {
        CMD_TAKE_ONE_ARG("Remove")
        usbCo->fileCommand(SD2Snes::opcode::RM, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::MakeDir : {
        CMD_TAKE_ONE_ARG("MakeDir")
        usbCo->fileCommand(SD2Snes::opcode::MKDIR, req->arguments.at(0).toLatin1());
        req->state = RequestState::WAITINGREPLY;
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





void    WSServer::processLowCoCmdFinished(USBConnection* usbco)
{
    LowCoInfos&  info = lowConnectionInfos[usbco];
    sDebug() << "Processing command finished" << info.currentCommand;
    switch (info.currentCommand) {
    case USB2SnesWS::Info :
    {
        USB2SnesInfo    ifo = usbco->parseInfo(usbco->dataRead);
        sendReply(info.currentWS, QStringList() << ifo.version << "foo" << ifo.romPlaying << ifo.flags);
        break;
    }

    // FILE Command
    case USB2SnesWS::List :
    {
        QList<USBConnection::FileInfos> lfi = usbco->parseLSCommand(usbco->dataRead);
        QStringList rep;
        foreach(USBConnection::FileInfos fi, lfi)
        {
            rep << QString::number((quint32) fi.type);
            rep << fi.name;
        }
        sendReply(info.currentWS, rep);
        break;
    }
    case USB2SnesWS::GetFile :
    {
        disconnect(usbco, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onLowCoGetDataReceived(QByteArray)));
        disconnect(usbco, SIGNAL(sizeGet(uint)), this, SLOT(onLowCoSizeGet(uint)));
        break;
    }
    case USB2SnesWS::Rename :
    case USB2SnesWS::Remove :
    case USB2SnesWS::MakeDir :
    case USB2SnesWS::PutFile :
    {
        break;
    }
    case USB2SnesWS::GetAddress :
    {
        disconnect(usbco, SIGNAL(getDataReceived(QByteArray)), this, SLOT(onLowCoGetDataReceived(QByteArray)));
        disconnect(usbco, SIGNAL(sizeGet(uint)), this, SLOT(onLowCoSizeGet(uint)));
        break;
    }
    default:
    {
        sDebug() << "Error, command not found";
        return;
    }
    }
    currentRequest[usbco]->state = RequestState::DONE;
    sDebug() << "Request " << *(currentRequest.value(usbco)) << "processed in " << currentRequest.value(usbco)->timeCreated.msecsTo(QTime::currentTime()) << " ms";
    delete currentRequest[usbco];
    currentRequest[usbco] = NULL;
}


QStringList WSServer::getDevicesList()
{
    QStringList toret;
    sDebug() << "Device List";
    foreach (USBConnection* lowCo, lowConnections)
        toret << lowCo->name();
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos) {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber() << "Busy : " << usbinfo.isBusy();
        if (usbinfo.isBusy() == false && !toret.contains(usbinfo.portName()))
            toret << usbinfo.portName();
    }
    return toret;
}

void WSServer::cmdAttach(MRequest *req)
{
    QString port = req->arguments.at(0);
    bool    portFound = false;
    foreach (USBConnection* bla, lowConnections) {
        if (bla->name() == port)
        {
            portFound = true;
            break;
        }
    }
    if (!portFound)
    {
        USBConnection* newCo = new USBConnection(port);
        lowConnectionInfos[newCo] = LowCoInfos();
        if (!newCo->open())
        {
            setError(ErrorType::CommandError, "Attach: Can't open the device on " + port);
            clientError(req->owner);
            return ;
        } else {
            sDebug() << "Added a new low Connection" << port;
            addNewLowConnection(newCo);
        }
    }
    foreach (USBConnection* lowCo, lowConnections)
    {
        if (lowCo->name() == port)
        {
            sDebug() << "Attaching " << wsNames.value(req->owner) <<  " to " << port;
            wsInfos[req->owner].attached = true;
            wsInfos[req->owner].attachedTo = lowCo;
            return ;
        }
    }
    setError(ErrorType::CommandError, "Trying to Attach to an unknow device");
    clientError(req->owner);
    return ;
}
