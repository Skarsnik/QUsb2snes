#ifndef RETROARCHDEVICE_H
#define RETROARCHDEVICE_H

#include "../adevice.h"
#include "retroarchhost.h"
#include "../rommapping/rominfo.h"

#include <QObject>
#include <QUdpSocket>
#include <QTimer>


struct RetroArchInfos {
    QString     gameName;
    QString     version;
    rom_type    romType;
    QString     error;
    uint        blockSize;
};

class RetroArchDevice : public ADevice
{
    Q_OBJECT
public:


    RetroArchDevice(RetroArchHost* mHost);
    /*RetroArchDevice(QUdpSocket* sock, QString raVersion, int bSize);
    RetroArchDevice(QUdpSocket* sock, QString raVersion, int bSize, QString gameName, enum rom_type romType);*/

    // ADevice interface
public:
    void fileCommand(SD2Snes::opcode op, QVector<QByteArray> args);
    void fileCommand(SD2Snes::opcode op, QByteArray args);
    void controlCommand(SD2Snes::opcode op, QByteArray args);
    void putFile(QByteArray name, unsigned int size);
    void getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void putAddrCommand(SD2Snes::space space, unsigned int addr0, unsigned int size);
    void putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size);
    void sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2);
    void infoCommand();
    void writeData(QByteArray data);
    bool canAttach();
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

    friend class RetroArchFactory;


public slots:
    bool open();
    void close();

private slots:
    void    onRHInfoDone(qint64 id);
    void    onRHInfoFailled(qint64 id);
    void    timedCommandDone();
    void    commandTimeout();
    void    onRHGetMemoryDone(qint64 id);

private:
    RetroArchHost*  host;
    QByteArray   dataToWrite;
    qint64       reqId;
    QString      hostName;

    bool         bigGet;
    unsigned int sizeBigGet;
    unsigned int sizeRequested;
    unsigned int sizePrevBigGet;
    unsigned int addrBigGet;
    unsigned int lastRCRSize;
    unsigned int sizePut;
    unsigned int sizeWritten;
    QByteArray   checkReturnedValue;

signals:
    void    checkReturned();
};

#endif // RETROARCHDEVICE_H
