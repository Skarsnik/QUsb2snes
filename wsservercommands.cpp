#include "wsserver.h"
#include <QLoggingCategory>
#include <QSerialPortInfo>

#define sDebug() qCDebug(log_wsserver)


void    WSServer::executeRequest(MRequest *req)
{
    USBConnection*  usbCo = NULL;
    QWebSocket*     ws = req->owner;
    sDebug() << "Executing request : " << req;
    if (wsInfos[ws].attached)
        usbCo = wsInfos[ws].attachedTo;
    switch (req->opcode)
    {
    case USB2SnesWS::DeviceList : {
        QStringList l = getDevicesList();
        sendReply(ws, l);
        break;
    }
    case USB2SnesWS::Attach : {
        cmdAttach(req);
        break;
    }
    case USB2SnesWS::AppVersion : {
        sendReply(ws, "7.42.0");
        break;
    }
    case USB2SnesWS::Name : {
        wsNames[ws] = req->arguments.at(0);
        break;
    }
    case USB2SnesWS::Info : {
        usbCo->infoCommand();
        req->state = RequestState::WAITINGREPLY;
        break;
    }
    case USB2SnesWS::List : {
        usbCo->fileCommand(SD2Snes::opcode::LS, req->arguments.at(0).toLatin1());
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

void    WSServer::processLowCoCmdFinished(USBConnection* usbco)
{
    LowCoInfos&  info = lowConnectionInfos[usbco];
    switch (info.currentCommand) {
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
    case USB2SnesWS::Info :
    {
        USB2SnesInfo    ifo = usbco->parseInfo(usbco->dataRead);
        sendReply(info.currentWS, QStringList() << ifo.version << "foo" << ifo.romPlaying << ifo.flags);
        break;
    }
    default:
    {
        sDebug() << "Error, command not found";
        return;
    }
    }
    currentRequest[usbco]->state = RequestState::DONE;
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
