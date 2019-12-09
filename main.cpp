#include "appui.h"
//#include "sd2snesdevice.h"
#include "wsserver.h"

#ifdef Q_OS_MACOS
#include "osx/appnap.h"
#endif

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
#include <QHostInfo>
#include <QVersionNumber>
#include <QStandardPaths>
#include <QDir>

WSServer    wsServer;
QSettings*  globalSettings;


static QTextStream logfile;
static QTextStream debugLogFile;
//static QTextStream lowlogfile;
static QTextStream cout(stdout);
//bool    dontLogNext = false;

bool    dontLogNext = false;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QTextStream*    log = &logfile;
    //cout << msg;
    if (dontLogNext)
        return ;
#ifdef QT_NO_DEBUG
    QString logString = QString("%6 %5 - %7: %1").arg(localMsg.constData()).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
#else
    QString logString = QString("%6 %5 - %7: %1 \t(%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
#endif
    switch (type)
    {
        case QtDebugMsg:
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
            QMessageBox::critical(nullptr, QObject::tr("Critical error"), msg);
            qApp->exit(1);
            break;
        case QtInfoMsg:
            *log << logString.arg("Info");
            break;
    }
    if (debugLogFile.device() != nullptr)
    {
        debugLogFile << logString.arg("Debug");
        debugLogFile << "\n";
        debugLogFile.flush();
    }
    if (type != QtDebugMsg)
    {
        *log << "\n";
        log->flush();
    }
    cout << QString("%1 : %2").arg(context.category, 20).arg(msg) << "\n";
    cout.flush();
}


void    startServer()
{
    //myTrayIcon->showMessage(QString("Webserver started"), QString("Webserver started"));
    quint16 port = 8080;
    QHostAddress addr = QHostInfo::fromName("localhost").addresses().first();
    if (globalSettings->contains("listen"))
        addr = QHostInfo::fromName(globalSettings->value("listen").toString()).addresses().first();
    if (globalSettings->contains("port"))
        port = globalSettings->value("port").toUInt();
    wsServer.start(addr, port);
}

int main(int ac, char *ag[])
{
    QApplication app(ac, ag);
#ifdef Q_OS_WIN
    QFile   mlog(qApp->applicationDirPath() + "/log.txt");
    QFile   mDebugLog(qApp->applicationDirPath() + "/log-debug.txt");
#else
    const QString appDataPath = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
    if (!QFile::exists(appDataPath))
        QDir().mkdir(appDataPath);
    QFile   mlog(appDataPath + "/log.txt");
    QFile   mDebugLog(appDataPath + "/log-debug.txt");
#endif

#ifndef Q_OS_WIN
    globalSettings = new QSettings("skarsnik.nyo.fr", "QUsb2Snes");
#else
    globalSettings = new QSettings("config.ini", QSettings::IniFormat);
#endif
    if (globalSettings->contains("debugLog") && globalSettings->value("debugLog").toBool())
    {
        mDebugLog.open(QIODevice::WriteOnly | QIODevice::Text);
        debugLogFile.setDevice(&mDebugLog);
    }
    if (mlog.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        logfile.setDevice(&mlog);
        qInstallMessageHandler(myMessageOutput);
    }
    QApplication::setApplicationName("QUsb2Snes");
    // This is only defined in the PRO file
#ifdef GIT_TAG_VERSION
    QString plop(GIT_TAG_VERSION);
    plop.remove(0, 1); // Remove the v
    QApplication::setApplicationVersion(QVersionNumber::fromString(plop).toString());
#else
    QApplication::setApplicationVersion("0.8");
#endif

#ifdef Q_OS_MACOS
    auto appNap = new AppNapSuspender();
    appNap->suspend();
#endif
    qDebug() << "Runing QUsb2Snes version " << QApplication::applicationVersion();
    // let set some know trusted domain
    wsServer.addTrusted("http://www.multitroid.com");
    wsServer.addTrusted("https://multiworld.samus.link/");

    if (app.arguments().size() == 2 && app.arguments().at(1) == "-nogui")
    {
        SD2SnesFactory* sd2snesFactory = new SD2SnesFactory();
        wsServer.addDeviceFactory(sd2snesFactory);
        if (globalSettings->value("retroarchdevice").toBool())
        {
            RetroArchFactory* retroarchFactory = new RetroArchFactory();
            wsServer.addDeviceFactory(retroarchFactory);
        }
        if (globalSettings->value("luabridge").toBool())
        {
            LuaBridge* luaBridge = new LuaBridge();
            wsServer.addDeviceFactory(luaBridge);
        }
        if (globalSettings->value("snesclassic").toBool())
        {
            SNESClassicFactory* snesClassic = new SNESClassicFactory();
            wsServer.addDeviceFactory(snesClassic);
        }
        QObject::connect(&wsServer, &WSServer::listenFailed, [=](const QString& err) {
            fprintf(stderr, "Can't listen on localhost:8080 or the host set in the config file: %s\n", err.toLatin1().data());
            qApp->exit(1);
        });
        QTimer::singleShot(100, &startServer);
        return app.exec();
    }
    AppUi*  appUi = new AppUi();
    appUi->sysTray->show();
    QTimer::singleShot(200, &startServer);
    return app.exec();
}
