#ifndef ADEVICE_H
#define ADEVICE_H

#include <QObject>
#include <QVector>
#include <QList>
#include "usb2snes.h"

class ADevice : public QObject
{
    Q_OBJECT
public:
    enum State {
      READY,
      BUSY,
      CLOSED
    };
    Q_ENUM(State)

    struct FileInfos {
        SD2Snes::file_type  type;
        QString             name;
    };

    explicit ADevice(QObject *parent = nullptr);

    virtual void            fileCommand(SD2Snes::opcode op, QVector<QByteArray> args) = 0;
    virtual void            fileCommand(SD2Snes::opcode op, QByteArray args) = 0;
    virtual void            controlCommand(SD2Snes::opcode op, QByteArray args = QByteArray()) = 0;
    virtual void            putFile(QByteArray name, unsigned int size) = 0;
    virtual void            getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) = 0;
    virtual void            putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) = 0;
    virtual void            putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args) = 0;
    virtual void            sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray &arg, const QByteArray arg2) = 0;
    virtual void            infoCommand() = 0;
    virtual void            writeData(QByteArray data) = 0;
    virtual QString         name() const = 0;
    virtual bool            hasFileCommands() = 0;
    virtual bool            hasControlCommands() = 0;
    virtual bool            canAttach() = 0;

    virtual USB2SnesInfo    parseInfo(const QByteArray &data) = 0;
    virtual QList<ADevice::FileInfos> parseLSCommand(QByteArray &dataI) = 0;
    QByteArray      dataRead;
    State           state() const;
    QString         attachError() const;

signals:
    void            commandFinished();
    void            protocolError();
    void            closed();
    void            getDataReceived(QByteArray data);
    void            sizeGet(unsigned int);

public slots:
    virtual bool    open() = 0;
    virtual void    close() = 0;

protected:
    State   m_state;
    QString m_attachError;
};

#endif // ADEVICE_H
