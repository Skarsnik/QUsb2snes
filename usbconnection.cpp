#include <QDebug>
#include <QLoggingCategory>
#include "usbconnection.h"

Q_LOGGING_CATEGORY(log_usbco, "USBCO")
#define sDebug() qCDebug(log_usbco)


USBConnection::USBConnection(QString portName)
{
    m_port.setPortName(portName);
    //m_port.baudRate();
    connect(&m_port, SIGNAL(readyRead()), this, SLOT(spReadyRead()));
    connect(&m_port, SIGNAL(aboutToClose()), this, SIGNAL(closed()));
    connect(&m_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(spErrorOccured(QSerialPort::SerialPortError)));
    m_state = CLOSED;
    m_getSize = -1;
    fileGetCmd = false;
    bytesReceived = 0;
}

bool USBConnection::open()
{
    bool toret = m_port.open(QIODevice::ReadWrite);
    m_port.setDataTerminalReady(true);
    sDebug() << "BaudRate : " << m_port.baudRate();
    sDebug() << "Databits : " << m_port.dataBits();
    sDebug() << "DataTerminalReady : " << m_port.isDataTerminalReady();
    sDebug() << "Parity : " << m_port.parity();
    sDebug() << "FlowControl : " << m_port.flowControl();
    sDebug() << "Stop bits : " << m_port.stopBits();
    if (toret)
        m_state = READY;
    return toret;
}

void USBConnection::close()
{
    m_port.setDataTerminalReady(false);
    m_port.close();
    m_state = CLOSED;
}

void USBConnection::spReadyRead()
{
    static QByteArray   responseBlock = QByteArray();

    bytesReceived += m_port.bytesAvailable();
    dataRead = m_port.readAll();
    if (dataRead.size() <= 2056)
    {
        sDebug() << "<<" << dataRead.size() << " bytes - total received : " << bytesReceived;
        sDebug() << dataRead;
    }
    else
        sDebug() << "<<" << dataRead.size() << " bytes - total received : " << bytesReceived;
    if (responseBlock.isEmpty() && (m_commandFlags & SD2Snes::server_flags::NORESP) == 0)
    {
        responseBlock = dataRead.left(512);
        if (responseBlock.left(5) != (QByteArray("USBA") + (char) SD2Snes::opcode::RESPONSE)
            || responseBlock.at(5) == 1)
        {
            sDebug() << "Protocol Error, invalid response block";
            emit protocolError();
            return;
        }
    }
    dataReceived.append(dataRead);
    if (fileGetCmd && m_getSize <= 0)
    {
        m_getSize = 0;
        sDebug() << responseBlock.mid(252, 4);
        m_getSize = ((quint32 ((quint8)(responseBlock.at(252))) << 24)) +
                    (((quint8) responseBlock.at(253)) << 16) +
                    (((quint8) responseBlock.at(254)) << 8)  +
                    responseBlock.at(255);
        sDebug() << "Size for data : " << m_getSize;
    }
    if (responseSizeExpected != -1)
    {
        if (bytesReceived == responseSizeExpected)
        {
            goto LcmdFinished;
        }
    } else {
        sDebug() << "Unsized command";
        if ((this->*checkCommandEnd)()) {
            goto LcmdFinished;
        }
    }
    return;
LcmdFinished :
    m_state = READY;
    dataRead = dataReceived;
    dataReceived.clear();
    bytesReceived = 0;
    fileGetCmd = false;
    responseBlock.clear();
    sDebug() << "Command finished";
    emit commandFinished();
}

void USBConnection::spErrorOccurred(QSerialPort::SerialPortError err)
{
    sDebug() << "Error " << err << m_port.errorString();
    emit closed();
}

bool USBConnection::checkEndForLs()
{
    if (dataReceived.size() == 512)
        return false;
    QByteArray data = dataReceived.mid(512);
    unsigned int cpt = 0;
    unsigned char type;
    while (cpt < data.size())
    {
        type = data.at(cpt);
        if (type == 0xFF)
            break;
        cpt++;
        while (cpt < data.size() && data.at(cpt) != 0)
            cpt++;
        cpt++;
    }
    if (cpt >= data.size())
        return false;
    return true;
}

bool    USBConnection::checkEndForGet()
{
    quint64 cmp_size = m_getSize;
    if (m_getSize % 512 != 0)
        cmp_size = (m_getSize / 512) * 512 + 512;
    sDebug() << cmp_size;
    return bytesReceived == cmp_size + 512;
}

void    USBConnection::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray& arg, const QByteArray arg2 = QByteArray())
{
    unsigned int filer_size = 512 - 7;
    m_commandFlags = flags;
    sDebug() << "CMD : " << opcode << space << flags << arg;
    QByteArray data("USBA");
    data.append((char) opcode);
    data.append((char) space);
    data.append((char) flags);
    data.append(QByteArray().fill(0, filer_size));
    data.replace(256, arg.size(), arg);
    if (!arg2.isEmpty())
        data.replace(252, arg2.size(), arg2);
    sDebug() << ">>" << data.left(8) << "- 252-272 : " << data.mid(252, 20);
    m_state = BUSY;
    m_currentCommand = opcode;
    writeData(data);
}

void USBConnection::infoCommand()
{
    responseSizeExpected = 512;
    sendCommand(SD2Snes::opcode::INFO, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, QByteArray());
}

void USBConnection::writeData(QByteArray data)
{
    //sDebug() << ">>" << data.size() << data;
    if (data.size() < 512)
        data.append(QByteArray().fill(0, 512 - data.size()));
    quint64 written = m_port.write(data);
    sDebug() << "Written : " << written << " bytes";
}

QString USBConnection::name() const
{
    return m_port.portName();
}


void    USBConnection::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    responseSizeExpected = 512;
    if (op == SD2Snes::opcode::LS)
    {
        responseSizeExpected = -1;
        checkCommandEnd = &USBConnection::checkEndForLs;
    }
    if (op == SD2Snes::opcode::GET)
    {
        responseSizeExpected = -1;
        m_getSize = 0;
        fileGetCmd = true;
        checkCommandEnd = &USBConnection::checkEndForGet;
    }
    sendCommand(op, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, args[0]);
}

void USBConnection::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    QVector<QByteArray> p;
    p.append(args);
    fileCommand(op, p);
}

void USBConnection::putFile(QByteArray name, unsigned int size)
{
    QByteArray data;
    responseSizeExpected = 512;
    data.append((char) (size >> 24) & 0xFF);
    data.append((char) (size >> 16) & 0xFF);
    data.append((char)(size >> 8) & 0xFF);
    data.append((char) size & 0xFF);
    m_currentCommand = SD2Snes::opcode::PUT;
    m_commandFlags = 0;
    sendCommand(SD2Snes::opcode::PUT, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, name, data);
}

QList<USBConnection::FileInfos>    USBConnection::parseLSCommand(QByteArray& dataI)
{
    QList<FileInfos>  infos;
    QByteArray data = dataI.mid(512);
    unsigned int cpt = 0;
    unsigned char type;
    while (true)
    {
        type = data.at(cpt);
        if (type == 0xFF)
            break;
        if (type == 0x02)
        {
            while (cpt % 512)
                cpt++;
        }
        QString name;
        cpt++;
        while (data.at(cpt) != 0)
        {
            name.append(data.at(cpt));
            cpt++;
        }
        cpt++;
        FileInfos fi;
        fi.type = (SD2Snes::file_type) type;
        fi.name = name;
        infos.append(fi);
    }
    return infos;
}

USB2SnesInfo USBConnection::parseInfo(const QByteArray& data)
{
    sDebug() << "Parse infos";
    USB2SnesInfo info;

    info.romPlaying = data.mid(16, 100);
    info.version = data.mid(260);
    sDebug() << QString::number(((data.at(256) << 24) | (data.at(257) << 16) | (data.at(258) << 8) | data.at(259)), 16);
    unsigned char flag = data.at(6);
    if ((flag & (char) SD2Snes::info_flags::FEAT_DSPX) != 0) info.flags.append("FEAT_DSPX");
    if ((flag & (char) SD2Snes::info_flags::FEAT_ST0010) != 0) info.flags.append("FEAT_ST0010");
    if ((flag & (char) SD2Snes::info_flags::FEAT_SRTC) != 0) info.flags.append("FEAT_SRTC");
    if ((flag & (char) SD2Snes::info_flags::FEAT_MSU1) != 0) info.flags.append("FEAT_MSU1");
    if ((flag & (char) SD2Snes::info_flags::FEAT_213F) != 0) info.flags.append("FEAT_213F");
    if ((flag & (char) SD2Snes::info_flags::FEAT_CMD_UNLOCK) != 0) info.flags.append("FEAT_CMD_UNLOCK");
    if ((flag & (char) SD2Snes::info_flags::FEAT_USB1) != 0) info.flags.append("FEAT_USB1");
    if ((flag & (char) SD2Snes::info_flags::FEAT_DMA1) != 0) info.flags.append("FEAT_DMA1");
    return info;
}

USBConnection::State USBConnection::state() const
{
    return m_state;
}
