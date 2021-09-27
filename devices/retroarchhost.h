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


#ifndef RETROARCHHOST_H
#define RETROARCHHOST_H

#include <QHostInfo>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVersionNumber>
#include <functional>
#include "utils/rommapping/rominfo.h"

class RetroArchHost : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        None,
        GetMemory,
        WriteMemory,
        ReqInfoVersion,
        ReqInfoStatus,
        ReqInfoRMemoryWRAM,
        ReqInfoRMemoryHiRomData,
        ReqInfoRMemoryLoRomData,
        ReqInfoRRAMZero,
        ReqInfoRRAMHiRomData,
        ReqInfoMemory2
    };

    struct Command
    {
        qint64 id;
        QByteArray cmd;
        State state;
        std::function<void(qint64)> sentCallback;
    };

    explicit    RetroArchHost(QString name, QObject *parent = nullptr);

    void            setHostAddress(QHostAddress, quint16 port = 55355);
    qint64          getMemory(unsigned int address, unsigned int size);
    qint64          getInfos();
    qint64          writeMemory(unsigned int address, unsigned int size);
    void            writeMemoryData(qint64 id, QByteArray data);
    QVersionNumber  version() const;
    QByteArray      getMemoryData() const;
    QString         name() const;
    QString         gameTitle() const;
    bool            hasRomAccess() const;
    bool            hasRomWriteAccess() const;
    QHostAddress    address() const;
    QString         lastInfoError() const;


signals:
    void    commandTimeout(qint64 id);
    void    errorOccured(QAbstractSocket::SocketError err);
    void    getMemoryDone(qint64 id);
    void    getMemoryFailed(qint64 id);
    void    writeMemoryDone(qint64 id);
    void    writeMemoryFailed(qint64 id);
    void    infoDone(qint64 id);
    void    infoFailed(qint64 id);

private:
    qint64          lastId;
    QVersionNumber  m_version;
    QString         m_name;
    quint16         m_port;
    QHostAddress    m_address;
    QString         m_lastInfoError;
    bool            readRamHasRomAccess;
    bool            readMemoryAPI;
    bool            readMemoryHasRomAccess;
    qint64          reqId;
    State           state;
    QTimer          commandTimeoutTimer;
    QList<Command>  commandQueue;
    rom_type        romType;
    QString         m_gameTile;
    QUdpSocket      socket;
    int             getMemorySize;
    QByteArray      getMemoryDatas;

    QByteArray      writeMemoryBuffer;
    int             writeSize;
    unsigned int    writeAddress;
    qint64          writeId;

    qint64  nextId();
    void    setInfoFromRomHeader(QByteArray data);
    void    makeInfoFail(QString error);
    void    onReadyRead();
    void    onPacket(QByteArray& data);
    void    onCommandTimerTimeout();
    int     translateAddress(unsigned int address);
    qint64  queueCommand(QByteArray cmd, State state, std::function<void(qint64)> sentCallback = nullptr, qint64 forceId=-1);
    void    runCommandQueue();
    void    doCommandNow(const Command& cmd);
    void    writeSocket(QByteArray data);
};

#endif // RETROARCHHOST_H
