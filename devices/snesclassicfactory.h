#ifndef SNESCLASSICFACTORY_H
#define SNESCLASSICFACTORY_H

#include <QObject>
#include <QTcpSocket>

#include "../devicefactory.h"

#include "snesclassic.h"

class SNESClassicFactory : public DeviceFactory
{
    Q_OBJECT
public:
    SNESClassicFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    ADevice *attach(QString deviceName);
    bool deleteDevice(ADevice *);
    QString status();
    QString name() const;
private:
    QTcpSocket*         socket = nullptr;
    QByteArray          canoePid;
    SNESClassic*        device = nullptr;
    QTimer              checkAliveTimer;
    unsigned int        romLocation = 0;
    unsigned int        sramLocation = 0;
    unsigned int        ramLocation = 0;
    QString             snesclassicIP;

    void executeCommand(QByteArray toExec);
    void writeSocket(QByteArray toWrite);
    QByteArray readCommandReturns(QTcpSocket *msocket);
    QByteArray readSocketReturns(QTcpSocket* msocket);

    void findMemoryLocations();
    bool checkStuff();
    void aliveCheck();

    bool hasValidMemory();
    void resetMemoryAddresses();
};

#endif // SNESCLASSICFACTORY_H
