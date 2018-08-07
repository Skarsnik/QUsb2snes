#ifndef SNESCLASSIC_H
#define SNESCLASSIC_H

#include "adevice.h"
#include "telnetconnection.h"

#include <QObject>
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
    void putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2);
    void infoCommand();
    void writeData(QByteArray data);
    QString name() const;
    bool hasFileCommands();
    bool hasControlCommands();
    USB2SnesInfo parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

public slots:
    bool open();
    void close();

private slots:
    void    onTimerOut();
    void    onTelnetCommandReturned(QByteArray data);
    void    onTelnetDisconnected();

private:
    QTimer  m_timer;
    TelnetConnection*   telCo;
    QString             canoePid;
    unsigned int        putAddr;
    unsigned int        putSize;
    unsigned int        romLocation;
    unsigned int        sramLocation;
    unsigned int        ramLocation;
    bool                cmdWasGet;

    void                findMemoryLocations();
};

#endif // SNESCLASSIC_H
