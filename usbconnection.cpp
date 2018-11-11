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
    connect(&m_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(spErrorOccurred(QSerialPort::SerialPortError)));
    connect(&m_port, SIGNAL(dataTerminalReadyChanged(bool)), this, SLOT(onDTRChanged(bool)));
    connect(&m_port, SIGNAL(requestToSendChanged(bool)), this, SLOT(onRTSChanged(bool)));
    m_state = CLOSED;
    m_getSize = 0;
    fileGetCmd = false;
    bytesReceived = 0;
}

bool USBConnection::open()
{
    bool toret = m_port.open(QIODevice::ReadWrite);
    sDebug() << "Opening Serial connection : " << toret;
    m_port.clear();
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
    if (m_state != CLOSED)
        m_port.close();
    m_state = CLOSED;
}

void USBConnection::spReadyRead()
{
    static QByteArray   responseBlock = QByteArray();
    static bool         firstBlock = true;

    bytesReceived += m_port.bytesAvailable();
    dataRead = m_port.readAll();
    if (dataRead.size() <= 2056)
    {
        sDebug() << "<<" << dataRead.size() << " bytes - total received : " << bytesReceived;
        /*for (unsigned int i = 0; i < dataRead.size(); i += 512)
        {
            sDebug() << dataRead.mid(i, 512);
            sDebug() << "---------";
        }*/
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
            m_state = READY;
            if (fileGetCmd)
                fileData = dataRead.mid(512, m_getSize);
            dataReceived.clear();
            bytesReceived = 0;
            fileGetCmd = false;
            firstBlock = true;
            m_getSize = -1;
            responseBlock.clear();
            emit protocolError();
            return;
        }
    }

    dataReceived.append(dataRead);
    if (m_currentCommand == SD2Snes::opcode::GET && m_getSize <= 0)
    {
        m_getSize = 0;
        sDebug() << responseBlock.mid(252, 4);
        m_getSize = ((quint32 ((quint8)(responseBlock.at(252))) << 24)) |
                    (quint32 ((quint8) responseBlock.at(253)) << 16) |
                    (quint32 ((quint8) responseBlock.at(254)) << 8)  |
                    ((quint8)(responseBlock.at(255)));
        sDebug() << "Size for data : " << m_getSize;
        emit sizeGet(m_getSize);
    }
    if (responseSizeExpected != -1)
    {
        if (bytesReceived == responseSizeExpected)
        {
            goto LcmdFinished;
        }
    } else {
        //sDebug() << "Unsized command";
        if ((this->*checkCommandEnd)()) {
            if (m_currentCommand == SD2Snes::opcode::GET)
            {
                if (dataRead.size() == dataReceived.size())
                    emit getDataReceived(dataRead.mid(512, m_getSize));
                else
                    emit getDataReceived(dataRead.left(bytesReceived - m_getSize));
            }
            goto LcmdFinished;
        } else {
            if (m_currentCommand == SD2Snes::opcode::GET)
            {
                if (firstBlock)
                    emit getDataReceived(dataRead.mid(512));
                else
                    emit getDataReceived(dataRead);
            }
        }
    }
    firstBlock = false;
    return;
LcmdFinished :
    m_state = READY;
    dataRead = dataReceived;
    if (fileGetCmd)
        fileData = dataRead.mid(512, m_getSize);
    dataReceived.clear();
    bytesReceived = 0;
    fileGetCmd = false;
    firstBlock = true;
    m_getSize = -1;
    responseBlock.clear();
    sDebug() << "Command finished";
    emit commandFinished();
}

void USBConnection::spErrorOccurred(QSerialPort::SerialPortError err)
{
    sDebug() << "Error " << err << m_port.errorString();
    if (err == QSerialPort::NoError)
        return ;
    if (m_state != CLOSED)
    {
        m_state = CLOSED;
        m_port.close();
    }
    emit closed();
}

void    USBConnection::onRTSChanged(bool set)
{
    sDebug() << "RTS changed : " << set;
}

void USBConnection::onDTRChanged(bool set)
{
    sDebug() << "DTR changed : " << set;
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
    //sDebug() << cmp_size;
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
    if (!arg2.isEmpty() && opcode != SD2Snes::opcode::MV)
        data.replace(252, arg2.size(), arg2);
    if (!arg2.isEmpty() && opcode == SD2Snes::opcode::MV)
        data.replace(8, arg2.size(), arg2);
    sDebug() << ">>" << data.left(8).toHex() << "- 252-272 : " << data.mid(252, 20).toHex();
    m_state = BUSY;
    m_currentCommand = opcode;
    writeData(data);
}

void    USBConnection::sendVCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags,
                                    const QList<QPair<unsigned int, quint8> >& args)
{
    unsigned int filer_size = 512 - 7;
    m_commandFlags = flags;
    sDebug() << "CMD : " << opcode << space << flags << args;
    QByteArray data("USBA");
    data.append((char) opcode);
    data.append((char) space);
    data.append((char) flags);
    data.append(QByteArray().fill(0, filer_size));
    unsigned int i = 0;
    foreach (auto infos, args) {
        data[32 + i * 4] = infos.second;
        data[33 + i * 4] = (infos.first >> 16) & 0xFF;
        data[34 + i * 4] = (infos.first >> 8) & 0xFF;
        data[35 + i * 4] = infos.first & 0xFF;
        i++;
    }
    sDebug() << "VCMD Sending : " << data;
    m_state = BUSY;
    m_currentCommand = opcode;
    writeData(data);
}

void USBConnection::infoCommand()
{
    responseSizeExpected = 512;
    sendCommand(SD2Snes::opcode::INFO, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, QByteArray());
}

bool USBConnection::canAttach()
{
    return true;
}

void USBConnection::writeData(QByteArray data)
{
    //sDebug() << ">>" << data.size() << data;
    if (data.size() < 512)
        data.append(QByteArray().fill(0, 512 - data.size()));
    if (data.size() % 512 != 0)
    {
        data.resize((data.size() / 512) * 512 + 512);
    }
    quint64 written = m_port.write(data);
    sDebug() << "Written : " << written << " bytes";
    m_port.flush();
}

QString USBConnection::name() const
{
    return "SD2SNES " + m_port.portName();
}

bool USBConnection::hasFileCommands()
{
    return true;
}

bool USBConnection::hasControlCommands()
{
    return true;
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
    if (args.size() != 2)
        sendCommand(op, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, args[0]);
    else
        sendCommand(op, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, args[0], args[1]);
}

void USBConnection::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    QVector<QByteArray> p;
    p.append(args);
    fileCommand(op, p);
}

void USBConnection::controlCommand(SD2Snes::opcode op, QByteArray args)
{
    responseSizeExpected = 512;
    sendCommand(op, SD2Snes::space::SNES, SD2Snes::server_flags::NONE, args);
}

static QByteArray   int24ToData(quint32 number)
{
    QByteArray data;
    data.append((char) (number >> 24) & 0xFF);
    data.append((char) (number >> 16) & 0xFF);
    data.append((char)(number >> 8) & 0xFF);
    data.append((char) number & 0xFF);
    sDebug() << "convertir numnber" << number << "to bitarray : " << data.toHex();
    return data;

}


void USBConnection::putFile(QByteArray name, unsigned int size)
{
    QByteArray data = int24ToData(size);
    responseSizeExpected = 512;
    m_currentCommand = SD2Snes::opcode::PUT;
    m_commandFlags = 0;
    sendCommand(SD2Snes::opcode::PUT, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, name, data);
}


void USBConnection::getSetAddrCommand(SD2Snes::opcode op, unsigned int addr, unsigned int size)
{

}

void USBConnection::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    responseSizeExpected = -1;
    m_getSize = 0;
    checkCommandEnd = &USBConnection::checkEndForGet;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::GET, space, SD2Snes::server_flags::NONE, data1, data2);
}

void USBConnection::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    responseSizeExpected = 512;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::PUT, space, SD2Snes::server_flags::NONE, data1, data2);
}

void USBConnection::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> >& args)
{
    responseSizeExpected = 512;
    sendVCommand(SD2Snes::opcode::VPUT, space, SD2Snes::server_flags::NONE, args);
}

void USBConnection::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    responseSizeExpected = 512;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::PUT, space, flags, data1, data2);
}

QList<ADevice::FileInfos>    USBConnection::parseLSCommand(QByteArray& dataI)
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

