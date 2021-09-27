/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
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

#ifndef SNESCLASSIC_H
#define SNESCLASSIC_H

#include "core/adevice.h"

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
