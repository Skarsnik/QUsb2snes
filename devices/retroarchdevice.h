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

#ifndef RETROARCHDEVICE_H
#define RETROARCHDEVICE_H

#include "../adevice.h"
#include "../rommapping/rominfo.h"

#include <QObject>
#include <QUdpSocket>
#include <qtimer.h>


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


    RetroArchDevice(QUdpSocket* sock, RetroArchInfos info, QString name);
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

    static RetroArchInfos   getRetroArchInfos(QUdpSocket* sock);
    friend class RetroArchFactory;


public slots:
    bool open();
    void close();

private slots:
    void    onUdpReadyRead();
    void    timedCommandDone();
    void    commandTimeout();

private:
    QUdpSocket*  m_sock;
    QByteArray   dataToWrite;
    QTimer*      m_timeout_timer;
    QString      m_raVersion;
    QString      m_gameName;
    QString      hostName;
    QHostAddress hostAddress;

    bool         bigGet;
    unsigned int sizeBigGet;
    unsigned int sizeRequested;
    unsigned int sizePrevBigGet;
    unsigned int addrBigGet;
    unsigned int lastRCRSize;
    unsigned int sizePut;
    unsigned int sizeWritten;
    bool         checkingRetroarch;
    bool         checkingInfo;
    bool         hasRomAccess;
    enum rom_type   romType;
    QByteArray   checkReturnedValue;
    rom_infos*   c_rom_infos;
    void         read_core_ram(unsigned int addr, unsigned int size);
    int          addr_to_addr(unsigned int addr);
    unsigned int blockSize;
    QString      m_uuid;

    void    create(QUdpSocket *sock, QString raVersion, int bSize);
signals:
    void    checkReturned();
};

#endif // RETROARCHDEVICE_H
