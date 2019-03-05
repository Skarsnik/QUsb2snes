#ifndef RETROARCHDEVICE_H
#define RETROARCHDEVICE_H

#include "../adevice.h"

#include <QObject>
#include <QUdpSocket>
#include <qtimer.h>


class RetroArchDevice : public ADevice
{
    Q_OBJECT
public:
    RetroArchDevice(QUdpSocket* sock);

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


public slots:
    bool open();
    void close();

private slots:
    void    onUdpReadyRead();
    void    timedCommandDone();

private:
    QUdpSocket*  m_sock;
    QByteArray   dataToWrite;
    QTimer*      m_timer;
    bool         bigGet;
    unsigned int sizeBigGet;
    unsigned int sizeRequested;
    unsigned int addrBigGet;
    unsigned int lastRCRSize;
    bool         checkingRetroarch;
    QByteArray   checkReturnedValue;
    void         read_core_ram(unsigned int addr, unsigned int size);

signals:
    void    checkReturned();
};

#endif // RETROARCHDEVICE_H
