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
    void fileCommand(SD2Snes::opcode op, QVector<QByteArray> args) override;
    void fileCommand(SD2Snes::opcode op, QByteArray args) override;
    void controlCommand(SD2Snes::opcode op, QByteArray args)  override;
    void putFile(QByteArray name, unsigned int size) override;
    void getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) override;
    void getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args) override;
    void putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) override;
    void putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args) override;
    void putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size) override;
    void infoCommand() override;
    void setState(ADevice::State state);
    void writeData(QByteArray data) override;
    QString name() const override;
    bool hasFileCommands() override;
    bool hasControlCommands() override;
    bool canAttach();
    void sockConnect(QString ip);
    USB2SnesInfo parseInfo(const QByteArray &data) override;
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI) override;
    void    setMemoryLocation(unsigned int ramLoc, unsigned int sramLoc, unsigned int romLoc);
    QByteArray          canoePid;
public slots:
    bool open() override;
    void close() override;

private slots:
    void    onTimerOut();
    void    onSocketReadReady();
    void    onSocketDisconnected();
    void    onAliveTimeout();

    void onSocketConnected();
private:
    QTimer              m_timer;
    QTimer              alive_timer;
    QTcpSocket*         socket;
    QString             myIp;

    unsigned int        putAddr;
    unsigned int        putSize;
    unsigned int        romLocation;
    unsigned int        sramLocation;
    unsigned int        ramLocation;
    bool                cmdWasGet;
    unsigned int        getSize;
    QByteArray          getData;
    bool                requestInfo;
    QByteArray          lastCmdWrite;
    QByteArray          lastPutWrite;
    struct rom_infos*   c_rom_infos;

    void                findMemoryLocations();
    void                executeCommand(QByteArray toExec);
    void                writeSocket(QByteArray toWrite);
    QByteArray          readCommandReturns(QTcpSocket *msocket);
};

#endif // SNESCLASSIC_H
