#include "emunetworkaccessdevice.h"


#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_emunwaccessdevice, "Emu NWA Device")
#define sDebug() qCDebug(log_emunwaccessdevice)
#define sInfo() qCInfo(log_emunwaccessdevice)

EmuNetworkAccessDevice::EmuNetworkAccessDevice(QString _name)
{
    myName = _name;
    emu = new EmuNWAccessClient(this);
    emu->connectToHost("127.0.0.1", 65400);
    connect(emu, &EmuNWAccessClient::connected, this, [=]() {
        sDebug() << "Connected";
        m_state = READY;
    });
    connect(emu, &EmuNWAccessClient::readyRead, this, &EmuNetworkAccessDevice::onEmuReadyRead, Qt::UniqueConnection);
    connect(emu, &EmuNWAccessClient::disconnected, this, [=]()
    {
       m_state = CLOSED;
       emit closed();
    });
    m_state = CLOSED;
    emuVersion.clear();
}


void EmuNetworkAccessDevice::onEmuReadyRead()
{
    static int step = 0;
    auto rep = emu->readReply();
    sDebug() << "Reply ready";
    sDebug() << rep;
    switch(currentCmd)
    {
        case USB2SnesWS::Info:
        {
            if (!emuVersion.isEmpty())
            {
                if (step == 0) {
                    if (rep["state"] == "no_game")
                    {
                        whatRunning = "";
                        emit commandFinished();
                        return ;
                    } else {
                        step = 1;
                        emu->cmdGameInfo();
                    }
                } else {
                    whatRunning = rep.toMap()["name"];
                    step = 0;
                    emit commandFinished();
                    return;
                }
            } else {
                sDebug() << "No emu info, checking them";
                if (step == 0)
                {
                    emu->cmdEmuInfo();
                    step = 1;
                    return ;
                }
                if (step == 1 && emuVersion.isEmpty())
                {
                    emuVersion = rep["version"];
                    emuName = rep["name"];
                    emu->cmdEmuStatus();
                    step = 0;
                }
            }
            break;
        }
        default:
            break;
    }
}





void EmuNetworkAccessDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
{
}

void EmuNetworkAccessDevice::putFile(QByteArray name, unsigned int size)
{
}

void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
}

void EmuNetworkAccessDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
}

void EmuNetworkAccessDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
}

void EmuNetworkAccessDevice::infoCommand()
{
    currentCmd = USB2SnesWS::Info;
    sDebug() << "Info command";
    emu->cmdEmuStatus();
}

void EmuNetworkAccessDevice::writeData(QByteArray data)
{
}

QString EmuNetworkAccessDevice::name() const
{
    return myName;
}

bool EmuNetworkAccessDevice::hasFileCommands()
{
    return false;
}

bool EmuNetworkAccessDevice::hasControlCommands()
{
    return false;
}

USB2SnesInfo EmuNetworkAccessDevice::parseInfo(const QByteArray &data)
{
    Q_UNUSED(data);
    USB2SnesInfo toret;
    toret.version = emuVersion;
    toret.deviceName = "NWAccess";
    if (whatRunning == "")
        toret.romPlaying = "/bin/menu.bin"; // Fall back to menu
    else
        toret.romPlaying = whatRunning;
    return toret;
}

QList<ADevice::FileInfos> EmuNetworkAccessDevice::parseLSCommand(QByteArray &dataI)
{
    Q_UNUSED(dataI);
    return QList<ADevice::FileInfos>();
}

bool EmuNetworkAccessDevice::open()
{
    m_state = READY;
    return true;
}

void EmuNetworkAccessDevice::close()
{
    if (m_state != CLOSED)
        emu->disconnectFromHost();
}


void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
}

void EmuNetworkAccessDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
}
