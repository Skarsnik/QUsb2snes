#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)

#include "appui.h"
#include "snesclassic.h"
#include "wsserver.h"



extern WSServer    wsServer;

AppUi::AppUi(QObject *parent) : QObject(parent)
{
    sysTray = new QSystemTrayIcon(QIcon(":/img/icon64x64.ico"));
    menu = new QMenu();
    menu->addAction("QUsb2Snes v" + QApplication::applicationVersion());
    menu->addSeparator();
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToshow()));
    retroarchDevice = NULL;
    luaBridgeDevice = NULL;

    deviceMenu = menu->addMenu("Devices");

    retroarchAction = new QAction("Enable RetroArch virtual device");
    retroarchAction->setCheckable(true);
    connect(retroarchAction, SIGNAL(triggered(bool)), this, SLOT(onRetroarchTriggered(bool)));
    deviceMenu->addAction(retroarchAction);

    luaBridgeAction = new QAction("Enable Lua bridge (snes9x-rr)");
    luaBridgeAction->setCheckable(true);
    connect(luaBridgeAction, SIGNAL(triggered(bool)), this, SLOT(onLuaBridgeTriggered(bool)));
    deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addSeparator();

    appsMenu = menu->addMenu("Applications");
    appsMenu->addAction("No applications");

    QObject::connect(menu->addAction("Exit"), &QAction::triggered, qApp, &QApplication::exit);
    sysTray->setContextMenu(menu);
    retroarchDevice = NULL;
    settings = new QSettings("config.ini", QSettings::IniFormat);
    if (settings->value("retroarchdevice").toBool())
    {
        retroarchDevice = new RetroarchDevice();
        wsServer.addDevice(retroarchDevice);
        retroarchAction->setChecked(true);
    }
    if (settings->value("luabridge").toBool())
    {
        luaBridgeDevice = new LuaBridge();
        wsServer.addDevice(luaBridgeDevice);
        luaBridgeAction->setChecked(true);
    }
    wsServer.addDevice(new SNESClassic());
    checkForApplications();
    //handleMagic2Snes("D:\\Project\\build-Magic2Snes-Desktop_Qt_5_11_0_MinGW_32bit-Debug\\debug\\");
    appsMenu->addSeparator();
    appsMenu->addAction("Remote Applications");
    appsMenu->addSeparator();
    appsMenu->addAction("Multitroid");
}

void AppUi::onRetroarchTriggered(bool checked)
{
    if (checked == true)
    {
        if (retroarchDevice == NULL)
            retroarchDevice = new RetroarchDevice();
        wsServer.addDevice(retroarchDevice);
    } else {
        wsServer.removeDevice(retroarchDevice);
    }
    settings->setValue("retroarchdevice", checked);
}

void AppUi::onLuaBridgeTriggered(bool checked)
{
    if (checked == true)
    {
        if (luaBridgeDevice == NULL)
            luaBridgeDevice = new LuaBridge();
        wsServer.addDevice(luaBridgeDevice);
    } else {
        wsServer.removeDevice(luaBridgeDevice);
    }
    settings->setValue("luabridge", checked);
}

void AppUi::onMenuAboutToshow()
{
    deviceMenu->clear();
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addSection("Devices state");
    auto piko = wsServer.getDevicesInfo();
    QMapIterator<QString, QStringList> it(piko);
    while (it.hasNext())
    {
        auto p = it.next();
        deviceMenu->addAction(p.key() + " : " + p.value().join(" - "));
    }
}

void AppUi::onAppsMenuTriggered(QAction *action)
{
    if (action->text() == "Multitroid")
    {
        QDesktopServices::openUrl(QUrl("http://multitroid2.lurern.com/"));
        /*QWebEngineView *view = new QWebEngineView(parent);
        view->load(QUrl("http://qt-project.org/"));
        view->show();*/
    }
    if (action->data().isNull())
        return ;
    sDebug() << "Running" << action->data().toString();
    QProcess::startDetached(action->data().toString());
}

void AppUi::onMagic2SnesMenuTriggered(QAction *action)
{
    if (!action->data().isNull())
        QProcess::startDetached(magic2SnesExe, QStringList() << action->data().toString());
    else
        QProcess::startDetached(magic2SnesExe);
}

void AppUi::checkForApplications()
{
    QDir    appsDir(qApp->applicationDirPath() + "/apps");
    if (!appsDir.exists())
        return ;
    foreach (QFileInfo fi, appsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        sDebug() << "Scanning " << fi;
        if (QFile::exists(fi.absoluteFilePath() + "/" + fi.baseName() + ".exe"))
        {
            regularApps[fi.baseName()] = fi.absoluteFilePath() + "/" + fi.baseName() + ".exe";
        } else {
            QDir dir(fi.absoluteFilePath());
            QFileInfoList fil = dir.entryInfoList(QDir::Files | QDir::Executable);
            if (!fil.isEmpty())
                regularApps[fi.baseName()] = fil.first().absoluteFilePath();
        }
    }
    if (regularApps.isEmpty())
        return ;
    appsMenu->clear();
    appsMenu->addAction("Local Applications");
    appsMenu->addSeparator();
    connect(appsMenu, SIGNAL(triggered(QAction*)), this, SLOT(onAppsMenuTriggered(QAction*)));
    QMapIterator<QString, QString> it(regularApps);
    while (it.hasNext())
    {
        it.next();
        sDebug() << "Adding " << it.key() << " - " << it.value();
        if (it.key() == "Magic2Snes")
            handleMagic2Snes(it.value());
        else
            appsMenu->addAction(it.key())->setData(it.value());
    }
}

void AppUi::handleMagic2Snes(QString path)
{
    magic2SnesMenu = appsMenu->addMenu("Magic2Snes");
    magic2SnesExe = path + "/Magic2Snes.exe";
    magic2SnesMenu->addAction("Run Magic2Snes");
    magic2SnesMenu->addSeparator();
    connect(magic2SnesMenu, SIGNAL(triggered(QAction*)), this, SLOT(onMagic2SnesMenuTriggered(QAction*)));
    QFileInfo fi(path);
    QString examplePath = fi.absolutePath() + "/examples";
    //QString examplePath = "D:\\Project\\Magic2Snes\\examples";
    //QString scriptPath = fi.absolutePath() + "/scripts";
    if (QFile::exists(examplePath))
    {
        QDir exDir(examplePath);
        auto fil = exDir.entryInfoList(QStringList() << "*.qml");
        foreach (QFileInfo efi, fil) {
            magic2SnesMenu->addAction(efi.fileName())->setData(efi.absoluteFilePath());
        }
        magic2SnesMenu->addAction("SMAlttp tracker")->setData(examplePath + "/smalttptracker/smalttpautotracker.qml");
    }
}
