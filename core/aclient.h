#pragma once

#include <QObject>
#include "adevice.h"
#include "types.h"

class AClient : public QObject
{
    Q_OBJECT

public:
    enum class ClientCommandState {
        NOCOMMAND,
        WAITINGREPLY, // The command want a reply
        WAITINGBDATAREPLY // The command want a binary reply
    };
    Q_ENUM(ClientCommandState)

    AClient(QObject* parent) : QObject(parent) {}

    QString                 name;
    bool                    attached;
    ADevice*                attachedTo;

    /* This handle all the weird stuff needed for putaddress
     You can have straight forward put request like that
     putAddress, then data
     or weird stuff like putAddress1, putAddress2, data1, data2
    */
    unsigned int            currentPutSize;
    QList<unsigned int>     pendingPutSizes;
    QList<QByteArray>       pendingPutDatas;
    QByteArray              recvData;
    unsigned int            byteReceived;

    // IPS Stuff
    QByteArray              ipsData;
    unsigned int            ipsSize;

    /* Attach not returning something, you can have for example
    Attach cmd then receive a getAddress before the Attach cmd is done
    for some device (like RA that check if RA is valable
    */
    bool                    pendingAttach;

    // This is to check if the client does what the current command
    // expect it to do
    ClientCommandState      commandState;

    /* This is used for 2 things:
     QUSB returns only its version if legacy if not, QUsb2snes-version
     For Websocket, this send a reply
    */
    bool                    legacy;


    virtual void                    sendData(QByteArray data) = 0;
    virtual void                    sendReply(QStringList list) = 0;
    virtual void                    close() = 0;
    virtual void                    setError(Core::ErrorType err, QString errString) = 0;

signals:
    void    closed();
    void    newRequest(Core::MRequest* req);
    void    binaryData(QByteArray data);
    void    errorOccured(Core::ErrorType err, QString message);

};
