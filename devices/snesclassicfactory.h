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
    QTcpSocket*         socket;
    QByteArray          canoePid;
    SNESClassic*        device;
    QTimer              checkAliveTimer;
    unsigned int        romLocation;
    unsigned int        sramLocation;
    unsigned int        ramLocation;
    QString             snesclassicIP;

    void executeCommand(QByteArray toExec);
    void writeSocket(QByteArray toWrite);
    QByteArray readCommandReturns(QTcpSocket *msocket);
    void findMemoryLocations();
    bool checkStuff();
    void aliveCheck();
};

#endif // SNESCLASSICFACTORY_H
