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

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QThread>
#include "sd2snesdevice.h"

Q_LOGGING_CATEGORY(log_sd2snes, "SD2SNES")
#define sDebug() qCDebug(log_sd2snes())


SD2SnesDevice::SD2SnesDevice(QString portName)
{
    m_port.setPortName(portName);
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


/*
 * USB2Snes firmware protocol is a bit simple
 * You send a block of 512 bytes with the command you want to do, arguments, flags...
 *
 * Then you receive a response block of blocksize if you did not specify no response
 * Then if a case of a command that give you data (ls, get...) you will receive
 * your data + padding to reach the block size
 * As: if you ask for 1060 byte, you will receive, 1054 byte + 512 bytes (the remaining 6 bytes then the padding)
 *
 * If you are writing data for example a put command, you also need to pad the data
 * to reach block size, for example writing 600 bytes, you send 512 + 88 and 424 bytes of padding.
 *
 * OPCODE, SPACE and FLAGS value are in the usb2snes.h file
 *
*/


/*
 *  To send a regular command the format is
 *  "USBA", OPCODE, SPACE, FLAGS, ..... (252) arg 2 (256) arg1
 *  Yes args are in the middle of the block
 *  Only the MV command place the second argument after the first sequence
 */

void    SD2SnesDevice::sendCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags, const QByteArray& arg, const QByteArray arg2 = QByteArray())
{
    int filer_size = 512 - 7;
    blockSize = 512;
    m_commandFlags = flags;
    sDebug() << "CMD : " << opcode << space << flags << arg;
    skipResponse = false;
    if (opcode == SD2Snes::opcode::PUT)
        skipResponse = true;
    QByteArray data("USBA");
    data.append(static_cast<char>(opcode));
    data.append(static_cast<char>(space));
    data.append(static_cast<char>(flags));
    data.append(QByteArray().fill(0, filer_size));
    //sDebug() << arg.toHex() << data.toHex();
    data.replace(256, arg.size(), arg);
    if (!arg2.isEmpty() && opcode != SD2Snes::opcode::MV)
        data.replace(252, arg2.size(), arg2);
    if (!arg2.isEmpty() && opcode == SD2Snes::opcode::MV)
        data.replace(8, arg2.size(), arg2);
    sDebug() << ">>" << data.left(8).toHex() << "- 252-272 : " << data.mid(252, 20).toHex();
    m_state = BUSY;
    m_currentCommand = opcode;
    writeToDevice(data);
}


/*
 * VPUT & VGET are a bit more complex as you need to use 64B block mode and noresp flags.
 *
 * then at byte 32 you put 1 byte size, 3 bytes for addr, 1 byte size2, 3 bytes addr 2, etc..
 */

void    SD2SnesDevice::sendVCommand(SD2Snes::opcode opcode, SD2Snes::space space, unsigned char flags,
                                    const QList<QPair<unsigned int, quint8> >& args)
{
    int filer_size = 64 - 7;
    // SD2Snes expect this flags for vget and vput
    flags |= SD2Snes::server_flags::DATA64B | SD2Snes::server_flags::NORESP;
    blockSize = 64;
    m_commandFlags = flags;
    sDebug() << "CMD : " << opcode << space << flags << args;
    QByteArray data("USBA");
    data.append(static_cast<char>(opcode));
    data.append(static_cast<char>(space));
    data.append(static_cast<char>(flags));
    data.append(QByteArray().fill(0, filer_size));
    int i = 0;
    int tsize = 0;
    foreach (auto infos, args) {
        data[32 + i * 4] = static_cast<char>(infos.second);
        data[33 + i * 4] = static_cast<char>((infos.first >> 16) & 0xFF);
        data[34 + i * 4] = static_cast<char>((infos.first >> 8) & 0xFF);
        data[35 + i * 4] = static_cast<char>(infos.first & 0xFF);
        i++;
        tsize += infos.second;
    }
    sDebug() << "VCMD Sending : " << data;
    if (opcode == SD2Snes::opcode::VGET)
        m_get_expected_size = tsize;
    if (opcode == SD2Snes::opcode::VPUT)
        m_putSize = tsize;
    m_state = BUSY;
    m_currentCommand = opcode;
    writeToDevice(data);
}



/*
 * Most command will return first a response block, this is mostly to validate the command
 * In a case of a GET command this response block will contains the size of data
 * returned by the GET (mostly usefull for file get)
 *  The response block look like "USBA", RESPONSE
 * For the INFO command, the response block contains the infos.
 *
 * Then you get your data relevant to the command :
 * Nothing for most command as a valid response block mean it's ok.
 * GET/VGET you get your data + padding to have a number of byte that are a multiple of blocksize (sig)
 * LS command return you a sequence of bytes like TYPE (1 byte), NAME
 *    0/1 are for file/directory, 02 mark that the name is in the next block (fuck this)
 *    FF is the end of the list.
*/

void SD2SnesDevice::spReadyRead()
{
    static QByteArray   responseBlock = QByteArray();
    static int          bytesGetSent = 0;
    static bool         fileGetSizeSent = false; // This avoid sending it twice

    QByteArray data = m_port.readAll();
    bytesReceived += data.size();
    dataReceived += data;

    sDebug() << "SP Received: " << data.size() << " (" << bytesReceived << ")";
    // Expecting a response block
    // We remove these data when we get the full block, that avoid lot of headache after
    if (responseBlock.isEmpty() && (m_commandFlags & SD2Snes::server_flags::NORESP) == 0)
    {
        if (bytesReceived >= blockSize)
        {
            responseBlock = dataReceived.left(blockSize);
            dataReceived.remove(0, blockSize);
            data = dataReceived;
            bytesReceived -= blockSize;
            if(responseBlock.left(5) != (QByteArray("USBA").append(SD2Snes::opcode::RESPONSE)) || responseBlock.at(5) == 1)
            {
                sDebug() << "Protocol error:" << responseBlock.left(6);
                m_state = READY;
                dataReceived.clear();
                bytesReceived = 0;
                fileGetCmd = false;
                m_getSize = -1;
                responseBlock.clear();
                emit protocolError();
                return;
            }
        } else {
            return;
        }
    }
    // Most command only need a valid response block
    if (m_currentCommand != SD2Snes::opcode::GET && m_currentCommand != SD2Snes::opcode::VGET
        && m_currentCommand != SD2Snes::opcode::LS)
    {
        if (m_currentCommand == SD2Snes::opcode::INFO)
            dataRead = responseBlock;
        if (skipResponse) // The firmware like to send me response block before a large put cmd is done?
        {
            responseBlock.clear();
            dataReceived.clear();
            bytesReceived = 0;
            skipResponse = false;
            return;
        }
        goto cmdFinished;
    } else {
        if (m_currentCommand == SD2Snes::opcode::LS)
        {
            lsData.append(data);
            if (checkEndForLs())
                goto cmdFinished;
            return;
        }
        if (bytesGetSent == 0) // before sending get data
        {
            if (!responseBlock.isEmpty())
            {
                m_getSize =  ((responseBlock.at(252)&0xFF) << 24);
                m_getSize += ((responseBlock.at(253)&0xFF) << 16);
                m_getSize += ((responseBlock.at(254)&0xFF) << 8);
                m_getSize += ((responseBlock.at(255)&0xFF));
                sDebug() << "Received block size:" << m_getSize;
            } else {
                m_getSize = m_get_expected_size;
            }
            if (fileGetCmd && !fileGetSizeSent)
            {
                emit sizeGet(m_getSize);
                fileGetSizeSent = true;
            }
        }

        // We want to send data ASAP
        /*
         * the issue we receive
         */
        // Remember the firmware pad data, we don't want to send the padding
        int firmwareBytesExpected = m_getSize;
        if (m_getSize % blockSize)
            firmwareBytesExpected = m_getSize + (blockSize - (m_getSize % blockSize));
        // We are fine, not dealing with the padding yet.
        if (bytesReceived != 0 && bytesReceived <= m_getSize)
        {
            bytesGetSent += data.size();
            emit getDataReceived(data);
        }
        if (bytesReceived > m_getSize)
        {
            QByteArray tmp = data.left(m_getSize - bytesGetSent);
            if (!tmp.isEmpty()) {
                bytesGetSent += tmp.size();
                emit getDataReceived(tmp);
            }
        }
        if (firmwareBytesExpected == bytesReceived)
        {
            bytesGetSent = 0;
            goto cmdFinished;
        }
    }
    return;
cmdFinished:
    fileGetSizeSent = false;
    m_state = READY;
    bytesReceived = 0;
    dataReceived.clear();
    responseBlock.clear();
    bytesGetSent = 0;
    m_getSize = 0;
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
    int cpt = 0;
    unsigned char type;
    sDebug() << lsData.size() << blockSize << lsData.size() % blockSize;
    if (lsData.size() < blockSize || lsData.size() % blockSize)
    {
        sDebug() << "Not reached the end of ls data";
        return false;
    }
    while (cpt < lsData.size())
    {
        type = static_cast<unsigned char>(lsData.at(cpt));
        if (type == 0xFF)
            break;
        cpt++;
        while (cpt < lsData.size() && lsData.at(cpt) != 0)
            cpt++;
        cpt++;
    }
    if (cpt >= lsData.size())
        return false;
    return true;
}

void SD2SnesDevice::infoCommand()
{
    sendCommand(SD2Snes::opcode::INFO, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, QByteArray());
}

bool SD2SnesDevice::canAttach()
{
    return true;
}

void SD2SnesDevice::writeToDevice(const QByteArray& data)
{
    static unsigned int writeCount = 0;
#ifdef Q_OS_MACOS
    QThread::msleep(10);
#endif
    auto written = m_port.write(data);
    sDebug() << "Written : " << data.toHex() << " || " << written << " bytes - " "Write count" << writeCount++;
#ifdef Q_OS_LINUX
    // Prevents a QSerialPort::ResourceError "Resource temporarily unavailable" error when uploading files to sd2snes.
    // This cause some other weird issue, need more testing?
    //    m_port.waitForBytesWritten();
#else
    //m_port.waitForBytesWritten();
    m_port.flush();
#endif
}

/* This is dumb and shoud not be needed */

void    SD2SnesDevice::beNiceToFirmWare(const QByteArray& data)
{
    int nbChunk = data.size() / blockSize;
    if (data.size() % blockSize)
        nbChunk += 1;
    for (unsigned int i = 0; i < nbChunk; i++)
    {
        writeToDevice(data.mid(i * blockSize, blockSize));
    }
}

void SD2SnesDevice::writeData(QByteArray data)
{
    static unsigned int dataSent = 0;
    unsigned int toWriteSize = data.size();
    sDebug() << toWriteSize << dataSent << m_putSize;
    // We are done sending stuff and this cover most case
    // Since only putfile is likely to send more than blocksize data
    if (dataSent + toWriteSize == m_putSize)
    {
        dataSent += toWriteSize;
        if (m_putSize % blockSize == 0)
            writeToDevice(data);
        else {
            sDebug() << "Adding padding to the write" << blockSize - (m_putSize % blockSize);
            writeToDevice(data + QByteArray(blockSize - (m_putSize % blockSize), 0));
        }
    } else {
        dataSent += data.size();
        writeToDevice(data);
    }
    sDebug() << "Putsize: " << m_putSize << " sendSize:" << toWriteSize;
    if (m_putSize != dataSent)
        return ;
    dataSent = 0;
    // Skipresponse is set to true for PUT cmd, only a new cmd or the reception of
    // a premature response block unset it
    if (skipResponse)
        skipResponse = false;
    else
    {
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

bool SD2SnesDevice::hasVariaditeCommands()
{
    return true;
}


void    SD2SnesDevice::fileCommand(SD2Snes::opcode op, QVector<QByteArray> args)
{
    if (op == SD2Snes::opcode::GET)
    {
        m_getSize = 0;
        fileGetCmd = true;
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
    sendCommand(op, SD2Snes::space::SNES, SD2Snes::server_flags::NONE, args);
}

static QByteArray   int32ToData(quint32 number)
{
    QByteArray data(4, 0);
    data[0] = static_cast<char>((number >> 24) & 0xFF);
    data[1] = static_cast<char>((number >> 16) & 0xFF);
    data[2] = static_cast<char>((number >> 8) & 0xFF);
    data[3] = static_cast<char>(number & 0xFF);
    //sDebug() << "convertir numnber" << number << "to bitarray : " << data.toHex();
    return data;

}


void SD2SnesDevice::putFile(QByteArray name, unsigned int size)
{
    QByteArray data = int32ToData(size);
    m_currentCommand = SD2Snes::opcode::PUT;
    m_commandFlags = 0;
    m_putSize = size;
    sendCommand(SD2Snes::opcode::PUT, SD2Snes::space::FILE, SD2Snes::server_flags::NONE, name, data);
}


void SD2SnesDevice::getSetAddrCommand(SD2Snes::opcode op, unsigned int addr, unsigned int size)
{
    Q_UNUSED(op);
    Q_UNUSED(addr);
    Q_UNUSED(size);
}

void SD2SnesDevice::getAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    m_getSize = 0;
    m_get_expected_size = static_cast<int>(size);
    QByteArray data1 = int32ToData(addr);
    QByteArray data2 = int32ToData(size);
    sendCommand(SD2Snes::opcode::GET, space, SD2Snes::server_flags::NONE, data1, data2);
}


void SD2SnesDevice::getAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> > &args)
{
    m_getSize = 0;
    m_get_expected_size = 0;
    foreach (auto p, args)
        m_get_expected_size += p.second;
    sendVCommand(SD2Snes::opcode::VGET, space, SD2Snes::server_flags::NONE, args);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, unsigned int addr, unsigned int size)
{
    QByteArray data1 = int32ToData(addr);
    QByteArray data2 = int32ToData(size);
    m_putSize = size;
    sendCommand(SD2Snes::opcode::PUT, space, SD2Snes::server_flags::NONE, data1, data2);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, QList<QPair<unsigned int, quint8> >& args)
{
    sendVCommand(SD2Snes::opcode::VPUT, space, SD2Snes::server_flags::NONE, args);
}

void SD2SnesDevice::putAddrCommand(SD2Snes::space space, unsigned char flags, unsigned int addr, unsigned int size)
{
    QByteArray data1 = int32ToData(addr);
    QByteArray data2 = int32ToData(size);
    m_putSize = size;
    sendCommand(SD2Snes::opcode::PUT, space, flags, data1, data2);
}


// Why we send the whole response?
QList<ADevice::FileInfos> SD2SnesDevice::parseLSCommand(QByteArray& dataI)
{
    Q_UNUSED(dataI)
    QList<FileInfos>  infos;
    QByteArray& data = lsData;
    int cpt = 0;
    unsigned char type;
    while (true)
    {
        type = static_cast<unsigned char>(data.at(cpt));
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
        fi.type = static_cast<SD2Snes::file_type>(type);
        fi.name = name;
        infos.append(fi);
    }
    lsData.clear();
    return infos;
}

USB2SnesInfo SD2SnesDevice::parseInfo(const QByteArray& data)
{
    sDebug() << "Parse infos";
    USB2SnesInfo info;

    info.deviceName = "SD2SNES";
    info.romPlaying = data.mid(16, 100);
    //info.romPlaying = info.romPlaying.mid(0, info.romPlaying.indexOf(QChar(0)));
    info.version = data.mid(260);
    info.version = info.version.mid(0, info.version.indexOf(QChar(0)));
    sDebug() << QString::number(((data.at(256) << 24) | (data.at(257) << 16) | (data.at(258) << 8) | data.at(259)), 16);
    unsigned char flag = static_cast<unsigned char>(data.at(6));
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_DSPX)) != 0) info.flags.append("FEAT_DSPX");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_ST0010)) != 0) info.flags.append("FEAT_ST0010");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_SRTC)) != 0) info.flags.append("FEAT_SRTC");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_MSU1)) != 0) info.flags.append("FEAT_MSU1");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_213F)) != 0) info.flags.append("FEAT_213F");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_CMD_UNLOCK)) != 0) info.flags.append("FEAT_CMD_UNLOCK");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_USB1)) != 0) info.flags.append("FEAT_USB1");
    if ((flag & static_cast<unsigned char>(SD2Snes::info_flags::FEAT_DMA1)) != 0) info.flags.append("FEAT_DMA1");
    return info;
}
