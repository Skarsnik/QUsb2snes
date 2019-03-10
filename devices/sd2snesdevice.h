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

    USB2SnesInfo    parseInfo(const QByteArray &data);
    QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI);

    QByteArray      fileData;



/*signals:
    void            commandFinished();
    void            protocolError();
    void            closed();
    void            getDataReceived(QByteArray data);
    void            sizeGet(unsigned int);*/

public slots:
    bool    open();
    void    close();

private slots:
    void    spReadyRead();
    void    spErrorOccurred(QSerialPort::SerialPortError err);
    void    onDTRChanged(bool set);
    void    onRTSChanged(bool set);

private:
    QSerialPort m_port;

    QByteArray  dataReceived;
    int         responseSizeExpected;
    int         bytesReceived;
    bool        fileGetCmd;
    bool        isGetCmd;
    quint16     blockSize;

    SD2Snes::opcode m_currentCommand;
    unsigned char   m_commandFlags;
    int             m_getSize;
    int             m_putSize;


    bool    (SD2SnesDevice::*checkCommandEnd)();

    bool    checkEndForLs();

    bool checkEndForGet();
};

#endif // USBCONNECTION_H
