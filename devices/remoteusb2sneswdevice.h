#ifndef REMOTEUSB2SNESWDEVICE_H
#define REMOTEUSB2SNESWDEVICE_H

#include <QtWebSockets/QtWebSockets>
#include <adevice.h>

class RemoteUsb2snesWDevice : public ADevice
{
    Q_OBJECT
public:
    explicit RemoteUsb2snesWDevice(QString remoteName, QObject *parent = nullptr);
    void     createWebsocket(QUrl url);
    void     sendText(const QString& message);
    void     sendBinary(const QByteArray& data);
    void     setClientName(const QString& name);


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

    struct Message {
        QString text;
        QByteArray datas;
    };
    QQueue<Message> queue;
    bool        attached = false;
    QWebSocket* websocket;
    QString     remoteDeviceName;
    QString     clientName;
    void     attach();
};

#endif // REMOTEUSB2SNESWDEVICE_H
