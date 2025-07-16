#ifndef REMOTEUSB2SNESWDEVICE_H
#define REMOTEUSB2SNESWDEVICE_H

#include <QWebsocket>
#include <adevice.h>

class RemoteUsb2snesWDevice : public ADevice
{
    Q_OBJECT
public:
    explicit RemoteUsb2snesWDevice(QObject *parent = nullptr);
    void     createWebsocket(QUrl url);
    void     send(QString message);
    void     send(QByteArray data);

    // ADevice interface
public:
    void fileCommand(SD2Snes::opcode op, QVector<QByteArray> args);
    void fileCommand(SD2Snes::opcode op, QByteArray args);
    void controlCommand(SD2Snes::opcode op, QByteArray args);
    void putFile(QByteArray name, unsigned int size);
    void getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size);
    void infoCommand();
    void writeData(QByteArray data);
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

signals:
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &message);
public slots:
    bool open();
    void close();

private:
    QWebSocket*  websocket;
};

#endif // REMOTEUSB2SNESWDEVICE_H
