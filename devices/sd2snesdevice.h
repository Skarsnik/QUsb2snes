/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet, Marcus Rowe.
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

#ifndef USBCONNECTION_H
#define USBCONNECTION_H

#include <QObject>
#include <QQueue>
#include <QSerialPort>
#include <QVector>
#include "adevice.h"

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
    struct VCmdEntry {
        QList<QPair<unsigned int, quint8> > list;
        unsigned int                        totalSIze;
    };

    QSerialPort m_port;

    // For handling reply
    QByteArray   responseBlock = QByteArray();
    int          bytesGetSent = 0;
    bool         fileGetSizeSent = false; // This avoid sending it twice

    unsigned int dataSent = 0;

    QByteArray  dataReceived;
    QByteArray  lsData;
    //int         responseSizeExpected;
    int         bytesReceived;
    bool        fileGetCmd;
    bool        isGetCmd;
    bool        skipResponse;
    quint16     blockSize;
    QQueue<VCmdEntry> vCmdArgumentsQueue;

    SD2Snes::opcode m_currentCommand;
    //VCmdEntry       m_currentVEntry;
    unsigned char   m_commandFlags;
    SD2Snes::space  m_currentSpace;
    int             m_getSize;
    int             m_putSize;
    int             m_get_expected_size;

    bool    checkEndForLs();

    void writeToDevice(const QByteArray &data);
    void beNiceToFirmWare(const QByteArray &data);
    QQueue<VCmdEntry> createSuitableListForVCMD(QList<QPair<unsigned int, quint8> > &originalArgs);
};

#endif // USBCONNECTION_H
