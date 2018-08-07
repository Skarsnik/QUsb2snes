#ifndef TELNETCONNECTION_H
#define TELNETCONNECTION_H

#include <QObject>
#include <QTcpSocket>

class TelnetConnection : public QObject
{
    Q_OBJECT
public:
    enum ConnectionError {
        ConnectionRefused,
        SNESNotRunning
    };
    enum State {
        Offline,
        Connected,
        Ready,
        WaitingForCommand
    };
    Q_ENUM(ConnectionError)
    Q_ENUM(State)

//private:
        enum InternalState {
            Init,
            AttemptConnection,
            SocketConnected,
            LoginWritten,
            Logged,
            PromptChangeWritten,
            PromptChanged,
            IReady,
            DataWritten,
            WaitingForCmd
        };
        Q_ENUM(InternalState)
public:
    TelnetConnection(const QString& hostname, int port, const QString& user, const QString& password);
    QByteArray  syncExecuteCommand(QString cmd);
    State       state();
    QString     debugName;
    void        setOneCommandMode(bool mode);

signals:
    void    connected();
    void    disconnected();
    void    error();
    void    commandReturn(QByteArray);
    void    commandReturnedNewLine(QByteArray);
    void    connectionError(TelnetConnection::ConnectionError);

public slots:
    void    conneect();
    void    executeCommand(QString toSend);
    void    close();

private slots:
    void    onSocketConnected();
    void    onSocketError(QAbstractSocket::SocketError error);
    void    onSocketDisconnected();
    void    onSocketReadReady();
private:
        QString m_host;
        int     m_port;
        QString m_user;
        QString m_password;
        QTcpSocket  socket;
        QByteArray lastSent;
        QByteArray readBuffer;
        QByteArray lastCommandReturn;
        State       m_state;
        InternalState m_istate;
        bool    oneCommandMode;
        unsigned int        nbRM;
        unsigned int        charToCheck;


        void    writeToTelnet(QByteArray toWrite);
        void    setState(State st);
};

#endif // TELNETCONNECTION_H
