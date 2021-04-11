/*
 * Copyright (c) 2021 the QUsb2Snes authors.
 *
 * This file is part of QUsb2Snes.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    virtual ~ADevice() {};
    virtual void            fileCommand(SD2Snes::opcode op, QVector<QByteArray> args) = 0;
    virtual void            fileCommand(SD2Snes::opcode op, QByteArray args) = 0;
    virtual void            controlCommand(SD2Snes::opcode op, QByteArray args = QByteArray()) = 0;
    virtual void            putFile(QByteArray name, unsigned int size) = 0;
    virtual void            getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) = 0;
    virtual void            getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args) = 0;
    virtual void            putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size) = 0;
    virtual void            putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args) = 0;
    virtual void            putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size) = 0;
    virtual void            infoCommand() = 0;
    virtual void            writeData(QByteArray data) = 0;
    virtual QString         name() const = 0;
    virtual bool            hasFileCommands() = 0;
    virtual bool            hasControlCommands() = 0;
    virtual bool            hasVariaditeCommands();
    virtual bool            deleteOnClose();

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

    QString getFlagString(USB2SnesWS::extra_info_flags flag);
};

#endif // ADEVICE_H
