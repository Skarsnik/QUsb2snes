#pragma once

#include <QObject>
#include <QTime>
#include "usb2snes.h"
class AClient;
//#include "aclient.h"

namespace Core {
    Q_NAMESPACE;

enum class RequestState {
    NEW,
    SENT,
    WAITINGREPLY,
    WAITINGBDATAREPLY,
    DONE,
    CANCELLED
};
Q_ENUM_NS(RequestState)

enum class ErrorType {
    CommandError,
    ProtocolError,
    DeviceError,
};
Q_ENUM_NS(ErrorType)

struct MRequest {
    MRequest() {
        id = gId++;
        wasPending = false;
    }
    quint64             id;
    AClient*            owner;
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


}
