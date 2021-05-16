#ifndef RETROARCHFACTORY_H
#define RETROARCHFACTORY_H

#include <QObject>
#include <QUdpSocket>
#include "../devicefactory.h"
#include "retroarchhost.h"
#include "retroarchdevice.h"


class RetroArchFactory : public DeviceFactory
{
    Q_OBJECT
    struct HostData {
        HostData() {
            device = nullptr;
        }
        QString             name;
        RetroArchHost*      host;
        RetroArchDevice*    device;
        qint64              reqId;
    };

public:
    RetroArchFactory();

    // DeviceFactory interface
public:
    QStringList listDevices();
    bool    hasAsyncListDevices();
    bool asyncListDevices();
    bool deleteDevice(ADevice *dev);
    QString status();
    QString name() const;
    ADevice* attach(QString deviceName);


private slots:
    void    onRaHostInfosDone(qint64 id);
    void    onRaHostgetInfosFailed(qint64 id);
    void    onRaHostErrorOccured(QAbstractSocket::SocketError err);
    void    onRaHostConnected();

private:
    QMap<QString, HostData> raHosts;
    int                     hostCheckCount;
    int                     hostChecked;

    void                    checkInfoDone();

/*    bool            checkRetroArchHost(RAHost& host);

    bool            tryNewRetroArchHost(RAHost& host);

    RAHost          testRetroArchHost(RAHost host);*/

    void    addHost(RetroArchHost *host);
};

#endif // RETROARCHFACTORY_H
