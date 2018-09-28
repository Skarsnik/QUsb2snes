#ifndef LUABRIDGE_H
#define LUABRIDGE_H

#include "adevice.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class LuaBridge : public ADevice
{
    Q_OBJECT
public:

    LuaBridge();

    // ADevice interface
public:
    void fileCommand(SD2Snes::opcode op, QVector<QByteArray> args);
    void fileCommand(SD2Snes::opcode op, QByteArray args);
    void controlCommand(SD2Snes::opcode op, QByteArray args);
    void putFile(QByteArray name, unsigned int size);
    void getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size);
    void sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2);
    void infoCommand();
    void writeData(QByteArray data);
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    bool canAttach();
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

public slots:
    bool open();
    void close();

private slots:
    void    onNewConnection();
    void    onServerError();
    void    onClientReadyRead();
    void    onClientDisconnected();
    void    onTimerOut();

private:
    QTcpServer* tcpServer;
    QTcpSocket* client;
    QTimer      timer;
    unsigned int putAddr;
    unsigned int putSize;
};

#endif // LUABRIDGE_H
