#include "usbconnection.h"
#include "wsserver.h"

#include <QSerialPortInfo>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QSystemTrayIcon>
#include <QFile>
#include <QMessageBox>
#include <QMenu>
#include <QObject>

USBConnection usbco("COM3");
WSServer    wsServer;
QSystemTrayIcon *myTrayIcon;

static QTextStream logfile;
//static QTextStream lowlogfile;
static QTextStream cout(stdout);
//bool    dontLogNext = false;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QTextStream*    log = &logfile;
    //cout << msg;
    QString logString = QString("%6 %5 - %7: %1 \t(%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    switch (type)
    {
        case QtDebugMsg:
            *log << logString.arg("Debug");
            break;
        case QtCriticalMsg:
            *log << logString.arg("Critical");
            break;
        case QtWarningMsg:
            *log << logString.arg("Warning");
            break;
        case QtFatalMsg:
            *log << logString.arg("Fatal");
            *log<< "\n"; log->flush();
            QMessageBox::critical(NULL, QObject::tr("Critical error"), msg);
            qApp->exit(1);
            break;
        case QtInfoMsg:
            *log << logString.arg("Info");
            break;
    }
    *log << "\n";
    log->flush();
    cout << QString("%1 : %2").arg(context.category, 20).arg(msg) << "\n";
    cout.flush();
}


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
    //myTrayIcon->showMessage(QString("Webserver started"), QString("Webserver started"));
    wsServer.start();
}

int main(int ac, char *ag[])
{
    QApplication app(ac, ag);
    QFile   mlog(qApp->applicationDirPath() + "/log.txt");
    logfile.setDevice(&mlog);
    if (mlog.open(QIODevice::WriteOnly | QIODevice::Text))
        qInstallMessageHandler(myMessageOutput);
    QApplication::setApplicationName("QUsb2Snes");
    myTrayIcon = new QSystemTrayIcon(QIcon(":/img/cheer.png"));
    QMenu* menu = new QMenu();
    //QAction aexit("Exit");
    QObject::connect(menu->addAction("Exit"), &QAction::triggered, &app, &QApplication::exit);
    myTrayIcon->setContextMenu(menu);

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
    myTrayIcon->setVisible(true);
    myTrayIcon->show();
    return app.exec();
}
