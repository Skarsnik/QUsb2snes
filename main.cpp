#include "usbconnection.h"
#include "wsserver.h"

#include <QSerialPortInfo>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QThread>

USBConnection usbco("COM3");
WSServer    wsServer;

void    myExit()
{
    usbco.close();
    exit(0);
    //qApp->exit(0);
}

void    myThing()
{
    QEventLoop  loop;
    loop.connect(&usbco, SIGNAL(commandFinished()), &loop, SLOT(quit()));
    if (usbco.dataRead.isEmpty())
    {
        qDebug() << "No data read for info";
        myExit();
    } else {
        USB2SnesInfo info = usbco.parseInfo(usbco.dataRead);
        qDebug() << info.romPlaying << info.version << info.flags;
    }
    usbco.fileCommand(SD2Snes::opcode::LS, "/");
    loop.exec();
    QList<USBConnection::FileInfos> lfi = usbco.parseLSCommand(usbco.dataRead);
    foreach (USBConnection::FileInfos fi, lfi) {
        qDebug() << fi.type << fi.name;
    }
    usbco.fileCommand(SD2Snes::opcode::LS, "/rando");
    loop.exec();
    lfi = usbco.parseLSCommand(usbco.dataRead);
    foreach (USBConnection::FileInfos fi, lfi) {
        qDebug() << fi.type << fi.name;
    }
    usbco.fileCommand(SD2Snes::opcode::GET, "/sd2snes/menu.bin");
    loop.exec();
    /*usbco.fileCommand(SD2Snes::opcode::MKDIR, "/temp");
    loop.exec();*/
    usbco.putFile("/temp/piko.txt", 11);
    usbco.writeData("Hello World");
    loop.exec();
    usbco.fileCommand(SD2Snes::opcode::GET, "/temp/piko.txt");
    loop.exec();
    myExit();
}

void    startServer()
{
    wsServer.start();
}

int main(int ac, char *ag[])
{
    QApplication app(ac, ag);
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos) {
        if (usbinfo.portName() == "COM3" && usbinfo.isBusy())
            return 0;
        qDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber() << "Busy : " << usbinfo.isBusy();
    }

    /*qDebug() << "Opening COMD3" << usbco.open();
    usbco.infoCommand();
    QTimer::singleShot(1000, &myThing);
    QTimer::singleShot(10000, &myExit);*/
    QTimer::singleShot(1000, &startServer);
    return app.exec();
}
