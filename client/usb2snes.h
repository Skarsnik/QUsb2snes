/*
    This file is part of the SaveState2snes software
    Copyright (C) 2017  Sylvain "Skarsnik" Colinet <scolinet@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef USB2SNES_H
#define USB2SNES_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>


#define USB2SNESURL "ws://localhost:8080/"


class USB2snes : public QObject
{
    Q_OBJECT
public:
    enum Space
    {
        SNES,
        CMD
    };
    enum State {
        None,
        Connected,
        Ready
    };
    enum sd2snesState {
        sd2menu,
        RomRunning
    };
    Q_ENUM(State)
    Q_ENUM(sd2snesState)
    // Should be private, but allow for Qt to register the enum
    enum InternalState {
        INone,
        IConnected,
        DeviceListRequested,
        AttachSent,
        FirmwareVersionRequested,
        ServerVersionRequested,
        IReady,
        IBusy
    };
    Q_ENUM(InternalState)

    USB2snes();
    void                    usePort(QString port);
    QString                 port();
    QString                 getRomName();
    void                    connect();
    void                    close();
    void                    setAppName(QString name);
    QByteArray              getAddress(unsigned int addr, unsigned int size, Space space = SNES);
    void                    setAddress(unsigned int addr, QByteArray data, Space space = SNES);
    State                   state();
    QStringList             infos();
    QString                 firmwareString();
    QVersionNumber          firmwareVersion();
    QStringList             deviceList();
    QVersionNumber          serverVersion();
    bool                    patchROM(QString patch);

signals:
    void    stateChanged();
    void    disconnected();
    void    binaryMessageReceived();
    void    textMessageReceived();
    void    romStarted();
    void    menuStarted();


private slots:
    void    onWebSocketConnected();
    void    onWebSocketDisconnected();
    void    onWebSocketTextReceived(QString message);
    void    onWebSocketBinaryReceived(QByteArray message);
    void    onWebSocketError(QAbstractSocket::SocketError err);
    void    onTimerTick();


private:
    QWebSocket      m_webSocket;
    QString         m_port;
    State           m_state;
    sd2snesState    m_sd2snesState;
    QVersionNumber  m_firmwareVersion;
    QString         m_firmwareString;
    QVersionNumber  m_serverVersion;
    InternalState   m_istate;
    QStringList     m_deviceList;
    QByteArray      lastBinaryMessage;
    QString         lastTextMessage;
    unsigned int    requestedBinaryReadSize;

    QTimer          timer;

    void            sendRequest(QString opCode, QStringList operands = QStringList(), Space = SNES, QStringList flags = QStringList());
    void            changeState(State s);
    QStringList     getJsonResults(QString json);

};

#endif // USB2SNES_H
