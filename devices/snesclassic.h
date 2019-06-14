#ifndef SNESCLASSIC_H
#define SNESCLASSIC_H

#include "../adevice.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class SNESClassic : public ADevice
{
    Q_OBJECT
public:
    SNESClassic();

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
    void setState(ADevice::State state);
    void writeData(QByteArray data);
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    bool canAttach();
    void sockConnect(QString ip);
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);
    void    setMemoryLocation(unsigned int ramLoc, unsigned int sramLoc, unsigned int romLoc);
    QByteArray          canoePid;
public slots:
    bool open();
    void close();

private slots:
    void    onTimerOut();
    void    onSocketReadReady();
    void    onSocketDisconnected();
    void    onAliveTimeout();

private:
    QTimer              m_timer;
    QTimer              alive_timer;
    QTcpSocket*         socket;

    unsigned int        putAddr;
    unsigned int        putSize;
    unsigned int        romLocation;
    unsigned int        sramLocation;
    unsigned int        ramLocation;
    bool                cmdWasGet;
    unsigned int        getSize;
    QByteArray          getData;
    bool                requestInfo;

    void                findMemoryLocations();
    void                executeCommand(QByteArray toExec);
    void                writeSocket(QByteArray toWrite);
    QByteArray          readCommandReturns(QTcpSocket *msocket);
};

#endif // SNESCLASSIC_H
