#ifndef RETROARCHHOST_H
#define RETROARCHHOST_H

#include <QHostInfo>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVersionNumber>
#include "../rommapping/rominfo.h"

class RetroArchHost : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        None,
        GetMemory,
        ReqInfoVersion,
        ReqInfoStatus,
        ReqInfoRMemoryWRAM,
        ReqInfoRMemoryHiRomData,
        ReqInfoRMemoryLoRomData,
        ReqInfoRRAMZero,
        ReqInfoRRAMHiRomData,
        ReqInfoMemory2
    };


    explicit    RetroArchHost(QString name, QObject *parent = nullptr);

    void            connectToHost(QHostAddress addr, quint16 port = 55355);
    void            connectToHost();
    void            setHostAddress(QHostAddress, quint16 port = 55355);
    qint64          getMemory(unsigned int address, unsigned int size);
    qint64          getInfos();
    void            write(QByteArray data);
    QVersionNumber  version() const;
    QByteArray      getMemoryData() const;
    QString         name() const;
    QString         gameTitle() const;
    bool            hasRomAccess() const;
    QHostAddress    address() const;
    bool            isConnected() const;


signals:
    void    connected();
    void    disconnected();
    void    connectionTimeout();
    void    errorOccured(QAbstractSocket::SocketError err);
    void    getMemoryDone(qint64 id);
    void    writeMemoryDone(qint64 id);
    void    infoDone(qint64 id);
    void    infoFailed(qint64 id);

private:
    qint64          id;
    QVersionNumber  m_version;
    QString         m_name;
    quint16         m_port;
    QHostAddress    m_address;
    QString         m_lastInfoError;
    bool            readRamHasRomAccess;
    bool            readMemory;
    State           state;
    QTimer          timeoutTimer;
    rom_type        romType;
    QString         m_gameTile;
    QUdpSocket      socket;
    int             getMemorySize;
    QByteArray      getMemoryDatas;
    bool            m_connected;

    void    setInfoFromRomHeader(QByteArray data);
    void    makeInfoFail(QString error);
    void    onReadyRead();
    int     translateAddress(unsigned int address);

};

#endif // RETROARCHHOST_H
