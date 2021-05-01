#ifndef EMUNETWORKACCESSDEVICE_H
#define EMUNETWORKACCESSDEVICE_H

#include <QObject>
#include "../adevice.h"
#include "emunwaccessclient.h"

class EmuNetworkAccessDevice : public ADevice
{
    Q_OBJECT

public:
    EmuNetworkAccessDevice(QString name);

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

public slots:
    bool open();
    void close();

private:
    EmuNWAccessClient*  emu;
    QString             emuName;
    QString             emuVersion;
    QString             whatRunning;
    QString             myName;


    unsigned int            getAddressSizeRequested;
    unsigned int            putAddressSize;
    unsigned int            putAddressSizeSent;
    QMap<QString, QString>  memoryAccess;
    std::function<void()>   afterMemoryAccess;

    USB2SnesWS::opcode  currentCmd;

    QPair<QString, unsigned int> sd2snesToDomain(unsigned int sd2snesAddr);
private slots:
    void onEmuReadyRead();
};

#endif // EMUNETWORKACCESSDEVICE_H
