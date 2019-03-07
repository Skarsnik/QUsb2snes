#include <QDebug>
#include <QLoggingCategory>
#include "sd2snesdevice.h"

Q_LOGGING_CATEGORY(log_sd2snes, "SD2SNES")
#define sDebug() qCDebug(log_sd2snes())


SD2SnesDevice::SD2SnesDevice(QString portName)
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
    blockSize = 512;
}

bool SD2SnesDevice::open()
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

void SD2SnesDevice::close()
{
    m_port.setDataTerminalReady(false);
    if (m_state != CLOSED)
        m_port.close();
    m_state = CLOSED;
}

void SD2SnesDevice::spReadyRead()
{
    static QByteArray   responseBlock = QByteArray();
    static bool         firstBlock = true;

    qint64 bytesToRead = m_port.bytesAvailable();
    bytesReceived += bytesToRead;
    dataRead = m_port.readAll();
    dataReceived.append(dataRead);

    sDebug() << "SP Received: " << bytesToRead << " (" << bytesReceived << ")";

    /* If we've received over 512 bytes and NORESP is not set, parse response header */
    if(responseBlock.isEmpty() && dataReceived.size() >= 512 && (m_currentCommand & SD2Snes::server_flags::NORESP) == 0)
    {
        responseBlock = dataReceived.left(512);
        dataReceived = dataReceived.mid(512);
        if(responseBlock.left(5) != (QByteArray("USBA") + (char)SD2Snes::opcode::RESPONSE) || responseBlock.at(5) == 1)
        {
            sDebug() << "Protocol error:" << responseBlock.left(6);
            m_state = READY;
            if(fileGetCmd)
                fileData = dataReceived.mid(0, m_getSize);
            dataRead.clear();
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

    if(m_getSize <= 0 && (m_currentCommand == SD2Snes::opcode::GET) && responseBlock.size() > 0)
    {
        m_getSize = (responseBlock.at(252)&0xFF) << 24;
        m_getSize += (responseBlock.at(253)&0xFF) << 16;
        m_getSize += (responseBlock.at(254)&0xFF) << 8;
        m_getSize += (responseBlock.at(255)&0xFF);
        sDebug() << "Received block size:" << m_getSize;
        emit sizeGet(m_getSize);
    }

    if(responseSizeExpected == -1)
    {
        if((this->*checkCommandEnd)() == false)
        {
            /* Still waiting for more data */
            if(dataReceived.size() >= 512)
            {
                if(m_getSize >= 0)
                {
                    emit getDataReceived(dataReceived.left(m_getSize));
                    m_getSize = -2;
                } else {
                    emit getDataReceived(dataReceived.left(512));
                }
                dataReceived = dataReceived.mid(512);

            }
            sDebug() << "Waiting for more data to arrive";
            return;
        }
    } else {
        if(bytesReceived != responseSizeExpected)
        {
            return;
        }
    }

    if(m_getSize != -2)
    {
        if(m_getSize >= 0)
        {
            emit getDataReceived(dataReceived.left(m_getSize));
        } else {
            emit getDataReceived(dataReceived);
        }
    }

    m_state = READY;
    dataRead = dataReceived;
    if (fileGetCmd)
    {
        fileData = dataRead.left(m_getSize);
    }
    dataRead.clear();
    dataReceived.clear();
    bytesReceived = 0;
    fileGetCmd = false;
    firstBlock = true;
    m_getSize = -1;
    responseBlock.clear();
    sDebug() << "Command finished";
    emit commandFinished();

}

void SD2SnesDevice::spErrorOccurred(QSerialPort::SerialPortError err)
{
    sDebug() << "Error " << err << m_port.errorString();
    if (err == QSerialPort::NoError)
        return ;
    if (m_state != CLOSED)
    {
        m_state = CLOSED;
        m_port.close();
    }
}

void    SD2SnesDevice::onRTSChanged(bool set)
{
    sDebug() << "RTS changed : " << set;
}

void SD2SnesDevice::onDTRChanged(bool set)
{
    sDebug() << "DTR changed : " << set;
}

bool SD2SnesDevice::checkEndForLs()
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

bool    SD2SnesDevice::checkEndForGet()
{
    quint64 cmp_size = m_getSize;
    if (m_getSize % blockSize != 0)
        cmp_size = (m_getSize / blockSize) * blockSize + blockSize;
    //sDebug() << cmp_size;
    if (m_commandFlags & SD2Snes::server_flags::NORESP)
        return bytesReceived == cmp_size;
    else
        return bytesReceived == cmp_size + blockSize;
}

void    SD2SnesDevice::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray& arg, const QByteArray arg2 = QByteArray())
{
    unsigned int filer_size = 512 - 7;
    blockSize = 512;
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

void    SD2SnesDevice::sendVCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags,
                                    const QList<QPair<unsigned int, quint8> >& args)
{
    unsigned int filer_size = 64 - 7;
    // SD2Snes expect this flags for vget and vput
    flags |= SD2Snes::server_flags::DATA64B | SD2Snes::server_flags::NORESP;
    blockSize = 64;
    m_commandFlags = flags;
    sDebug() << "CMD : " << opcode << space << flags << args;
    QByteArray data("USBA");
    data.append((char) opcode);
    data.append((char) space);
    data.append((char) flags);
    data.append(QByteArray().fill(0, filer_size));
    unsigned int i = 0;
    unsigned int tsize = 0;
    foreach (auto infos, args) {
        data[32 + i * 4] = infos.second;
        data[33 + i * 4] = (infos.first >> 16) & 0xFF;
        data[34 + i * 4] = (infos.first >> 8) & 0xFF;
        data[35 + i * 4] = infos.first & 0xFF;
        i++;
        tsize += infos.second;
    }
    sDebug() << "VCMD Sending : " << data;
    if (opcode == SD2Snes::opcode::VGET)
        m_getSize = tsize;
    if (opcode == SD2Snes::opcode::VPUT)
        m_putSize = tsize + blockSize;
    m_state = BUSY;
    m_currentCommand = opcode;
    writeData(data);
}

void SD2SnesDevice::infoCommand()
{
    responseSizeExpected = 512;
    sendCommand(SD2Snes::opcode::INFO, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, QByteArray());
}

bool SD2SnesDevice::canAttach()
{
    return true;
}

void SD2SnesDevice::writeData(QByteArray data)
{
    //sDebug() << ">>" << data.size() << data;
    auto sendSize = data.size();

    if (data.size() < blockSize)
        data.append(QByteArray().fill(0, blockSize - data.size()));
    if (data.size() % blockSize != 0)
    {
        data.resize((data.size() / blockSize) * blockSize + blockSize);
    }
    quint64 written = m_port.write(data);
    sDebug() << "Written : " << written << " bytes";
    m_port.flush();
    if (m_currentCommand == SD2Snes::VPUT)
    {
        sDebug() << "Putsize: " << m_putSize << " sendSize:" << sendSize;
        m_putSize -= sendSize;
        if (m_putSize != 0)
            return ;

        m_state = READY;
        dataRead = dataReceived;
        dataReceived.clear();
        bytesReceived = 0;
        fileGetCmd = false;
        m_getSize = -1;
        sDebug() << "Command finished";
        emit commandFinished();
    }
}

QString SD2SnesDevice::name() const
{
    return "SD2SNES " + m_port.portName();
}

bool SD2SnesDevice::hasFileCommands()
{
    return true;
}

bool SD2SnesDevice::hasControlCommands()
{
    return true;
}


void    SD2SnesDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    responseSizeExpected = 512;
    if (op == SD2Snes::opcode::LS)
    {
        responseSizeExpected = -1;
        checkCommandEnd = &SD2SnesDevice::checkEndForLs;
    }
    if (op == SD2Snes::opcode::GET)
    {
        responseSizeExpected = -1;
        m_getSize = 0;
        fileGetCmd = true;
        checkCommandEnd = &SD2SnesDevice::checkEndForGet;
    }
    if (args.size() != 2)
        sendCommand(op, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, args[0]);
    else
        sendCommand(op, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, args[0], args[1]);
}

void SD2SnesDevice::fileCommand(SD2Snes::opcode op, QByteArray args)
{
    QVector<QByteArray> p;
    p.append(args);
    fileCommand(op, p);
}

void SD2SnesDevice::controlCommand(SD2Snes::opcode op, QByteArray args)
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


void SD2SnesDevice::putFile(QByteArray name, unsigned int size)
{
    QByteArray data = int24ToData(size);
    responseSizeExpected = 512;
    m_currentCommand = SD2Snes::opcode::PUT;
    m_commandFlags = 0;
    sendCommand(SD2Snes::opcode::PUT, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, name, data);
}


void SD2SnesDevice::getSetAddrCommand(SD2Snes::opcode op, unsigned int addr, unsigned int size)
{

}

void SD2SnesDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    responseSizeExpected = -1;
    m_getSize = 0;
    checkCommandEnd = &SD2SnesDevice::checkEndForGet;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::GET, space, SD2Snes::server_flags::NONE, data1, data2);
}


void SD2SnesDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    responseSizeExpected = -1;
    m_getSize = 0;
    checkCommandEnd = &SD2SnesDevice::checkEndForGet;
    sendVCommand(SD2Snes::opcode::VGET, space, SD2Snes::server_flags::NONE, args);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    responseSizeExpected = 512;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::PUT, space, SD2Snes::server_flags::NONE, data1, data2);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> >& args)
{
    responseSizeExpected = 512;
    sendVCommand(SD2Snes::opcode::VPUT, space, SD2Snes::server_flags::NONE, args);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    responseSizeExpected = 512;
    QByteArray data1 = int24ToData(addr);
    QByteArray data2 = int24ToData(size);
    sendCommand(SD2Snes::opcode::PUT, space, flags, data1, data2);
}

QList<ADevice::FileInfos>    SD2SnesDevice::parseLSCommand(QByteArray& dataI)
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

USB2SnesInfo SD2SnesDevice::parseInfo(const QByteArray& data)
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
