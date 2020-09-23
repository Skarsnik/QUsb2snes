#ifndef USBCONNECTION_H
#define USBCONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QVector>
#include "../adevice.h"

class SD2SnesDevice : public ADevice
{
    Q_OBJECT
public:

    explicit SD2SnesDevice(QString portName);

    void            fileCommand(SD2Snes::opcode op, QVector<QByteArray> args);
    void            fileCommand(SD2Snes::opcode op, QByteArray args);
    void            controlCommand(SD2Snes::opcode op, QByteArray args = QByteArray());
    void            putFile(QByteArray name, unsigned int size);
    void            getSetAddrCommand(SD2Snes::opcode op, unsigned int addr, unsigned int size);
    void            getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void            getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void            putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size);
    void            putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args);
    void            putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size);
    void            sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2);
    void            sendVCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QList<QPair<unsigned int, quint8> > &args);
    void            infoCommand();
    bool            canAttach();
    void            writeData(QByteArray data);
    QString         name() const;

    bool            hasFileCommands();
    bool            hasControlCommands();
    bool            hasVariaditeCommands();

    USB2SnesInfo    parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

    QByteArray      fileData;


public slots:
    bool    open();
    void    close();

private slots:
    void    spReadyRead();
    void    spErrorOccurred(QSerialPort::SerialPortError err);
    void    onDTRChanged(bool set);
    void    onRTSChanged(bool set);

private:
    void    readPacket(const QByteArray& packetData);

private:
    QSerialPort m_port;

    QByteArray  dataReceived;
    QByteArray  lsData;
    //int         responseSizeExpected;
    int         bytesReceived;
    bool        fileGetCmd;
    bool        isGetCmd;
    bool        skipResponse;
    quint16     blockSize;

    SD2Snes::opcode m_currentCommand;
    unsigned char   m_commandFlags;
    int             m_getSize;
    int             m_putSize;
    int             m_get_expected_size;

    bool    checkEndForLs();

    void writeToDevice(const QByteArray &data);
    void beNiceToFirmWare(const QByteArray &data);
};

#endif // USBCONNECTION_H
