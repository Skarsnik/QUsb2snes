/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet, Thomas Backmark.
 *
 * This file is part of the QUsb2Snes project.
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

#ifndef RETROARCHDEVICE_H
#define RETROARCHDEVICE_H

#include "core/adevice.h"
#include "retroarchhost.h"
#include "utils/rommapping/rominfo.h"

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
    void    onRHGetMemoryDone(qint64 id);
    void    onRHWriteMemoryDone(qint64 id);
    void    onRHCommandTimeout(qint64 id);
    void    onRHGetMemoryFailed(qint64 id);

private:
    RetroArchHost*  host;
    qint64       reqId;
    QString      hostName;

signals:
    void    checkReturned();
};

#endif // RETROARCHDEVICE_H
