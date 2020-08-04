#ifndef LUABRIDGEDEVICE_H
#define LUABRIDGEDEVICE_H

#include "../adevice.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "../rommapping/rommapping.h"


class LuaBridgeDevice : public ADevice
{
    Q_OBJECT
public:

    LuaBridgeDevice(QTcpSocket* m_socket, QString name);

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
    void sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2);
    void infoCommand();
    void writeData(QByteArray data);
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);
    QTcpSocket* socket();
    QString luaName() const;

public slots:
    bool open();
    void close();

private slots:
    void    onServerError();
    void    onClientReadyRead();
    void    onClientDisconnected();
    void    onTimerOut();

private:
    QTcpSocket*     m_socket;
    QTimer          timer;
    QString         m_name;
    unsigned int    putAddr;
    unsigned int    putSize;
    bool            bizhawk;
    enum rom_type   romMapping;
    QString         gameName;
    bool            infoReq;

    void getRomMapping();
    QPair<QByteArray, unsigned int> getBizHawkAddress(unsigned int usbAddr);
    unsigned int getSnes9xAddress(unsigned int addr);
};

#endif // LUABRIDGEDEVICE_H
