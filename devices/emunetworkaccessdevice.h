#ifndef EMUNETWORKACCESSDEVICE_H
#define EMUNETWORKACCESSDEVICE_H

#include <QObject>
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
    void onEmuDisconnected();
};

#endif // EMUNETWORKACCESSDEVICE_H
