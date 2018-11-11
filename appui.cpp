#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QStyle>

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
    menu->addAction(QIcon(":/img/icon64x64.ico"), "QUsb2Snes v" + QApplication::applicationVersion());
    menu->addSeparator();
    retroarchDevice = nullptr;
    luaBridgeDevice = nullptr;
    snesClassicDevice = nullptr;

    deviceMenu = menu->addMenu(QIcon(":/img/deviceicon.svg"), "Devices");
    connect(deviceMenu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToshow()));


    retroarchAction = new QAction(QIcon(":/img/retroarch.png"), "Enable RetroArch virtual device (snes9x 2010 core only)");
    retroarchAction->setCheckable(true);
    connect(retroarchAction, SIGNAL(triggered(bool)), this, SLOT(onRetroarchTriggered(bool)));

    luaBridgeAction = new QAction(QIcon(":/img/icone-snes9x.gif"), "Enable Lua bridge (snes9x-rr)");
    luaBridgeAction->setCheckable(true);
    connect(luaBridgeAction, SIGNAL(triggered(bool)), this, SLOT(onLuaBridgeTriggered(bool)));

    snesClassicAction = new QAction(QIcon(":/img/chrysalis.png"), "Enable SNES Classic support (experimental)");
    snesClassicAction->setCheckable(true);
    connect(snesClassicAction, SIGNAL(triggered(bool)), this, SLOT(onSNESClassicTriggered(bool)));

    appsMenu = menu->addMenu(QIcon(":/img/appicon.svg"), "Applications");
    appsMenu->addAction("No applications");

    sysTray->setContextMenu(menu);
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
    if (settings->value("snesclassic").toBool())
    {
        snesClassicDevice = new SNESClassic();
        wsServer.addDevice(snesClassicDevice);
        snesClassicAction->setChecked(true);
    }
    checkForApplications();
    menu->addSeparator();
    handleMagic2Snes(qApp->applicationDirPath() + "/Magic2Snes");
    menu->addSeparator();
    QObject::connect(menu->addAction("Exit"), &QAction::triggered, qApp, &QApplication::exit);
    appsMenu->addSeparator();
    appsMenu->addAction("Remote Applications");
    appsMenu->addSeparator();
    appsMenu->addAction(QIcon(":/img/multitroid.png"), "Multitroid");
}

void AppUi::onRetroarchTriggered(bool checked)
{
    if (checked == true)
    {
        if (retroarchDevice == nullptr)
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
        if (luaBridgeDevice == nullptr)
            luaBridgeDevice = new LuaBridge();
        wsServer.addDevice(luaBridgeDevice);
    } else {
        wsServer.removeDevice(luaBridgeDevice);
    }
    settings->setValue("luabridge", checked);
}

void AppUi::onSNESClassicTriggered(bool checked)
{
    if (checked == true)
    {
        if (snesClassicDevice == nullptr)
            snesClassicDevice = new SNESClassic();
        wsServer.addDevice(snesClassicDevice);
    } else {
        wsServer.removeDevice(snesClassicDevice);
    }
    settings->setValue("snesclassic", checked);
}

void AppUi::onMenuAboutToshow()
{
    deviceMenu->clear();
    deviceMenu->addAction("Devices state");
    deviceMenu->addSeparator();
    auto piko = wsServer.getDevicesInfo();
    if (piko.isEmpty())
        deviceMenu->addAction("No device")->setEnabled(false);
    foreach(WSServer::MiniDeviceInfos dInfo, piko)
    {
        if (dInfo.usable)
        {
            if (dInfo.clients.isEmpty())
                deviceMenu->addAction(dInfo.name + " - No client connected");
            else
                deviceMenu->addAction(dInfo.name + " : " + dInfo.clients.join(" - "));
        } else {
            deviceMenu->addAction(dInfo.name + " - " + dInfo.error)->setEnabled(false);
        }
    }
    deviceMenu->addSeparator();
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addAction(snesClassicAction);
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
        if (QFile::exists(QFileInfo(it.value()).absolutePath() + "/icone.png"))
            appsMenu->addAction(QIcon(QFileInfo(it.value()).absolutePath() + "/icone.png"), it.key())->setData(it.value());
        else
            appsMenu->addAction(it.key())->setData(it.value());
    }
}


/*
 * MAGIC2SNES Stuff
*/


void AppUi::onMagic2SnesMenuTriggered(QAction *action)
{
    if (!action->data().isNull() && action->data().toString() == "DDDDIR")
        return ;
    if (!action->data().isNull())
        QProcess::startDetached(magic2SnesExe, QStringList() << action->data().toString());
    else
        QProcess::startDetached(magic2SnesExe);
}


static bool    searchForQUsb2snesEntry(QString qmlFile)
{
    QFile f(qmlFile);
    if (f.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        while (true)
        {
            QByteArray line = f.readLine();
            if (line.isEmpty())
                return false;
            if (line.indexOf("USB2Snes") != -1)
                return true;
        }
    }
    return false;
}

static QString  searchWindowTitle(QString qmlFile)
{
    QFile f(qmlFile);
    sDebug() << "Searching in " << qmlFile;
    if (f.open(QIODevice::Text | QIODevice::ReadOnly))
    {
        while (true)
        {
            QByteArray line = f.readLine();
            if (line.isEmpty())
                break;
            if (line.indexOf("windowTitle") != -1)
            {
                sDebug() << line;
                QRegExp exp("windowTitle\\s*\\:\\s*\"(.+)\"");
                if (exp.indexIn(line) != -1)
                {
                    sDebug() << "Found : " << exp.cap(1);
                    return exp.cap(1);
                }
            }
         }
    }
    return QString();
}

void AppUi::addMagic2SnesFolder(QString path)
{
    sDebug() << "Adding magic2snes qml path " << path;
    QFileInfo fi(path);
    QDir exDir(path);
    magic2SnesMenu->addSeparator();
    magic2SnesMenu->addAction(menu->style()->standardIcon(QStyle::SP_DirOpenIcon), fi.baseName())->setData("DDDDIR");
    magic2SnesMenu->addSeparator();
    auto fil = exDir.entryInfoList(QStringList() << "*.qml");
    foreach (QFileInfo efi, fil)
    {
        QString wTitle = searchWindowTitle(efi.absoluteFilePath());
        if (wTitle.isEmpty())
            magic2SnesMenu->addAction(efi.fileName())->setData(efi.absoluteFilePath());
        else
            magic2SnesMenu->addAction(wTitle)->setData(efi.absoluteFilePath());
    }
    fil = exDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
    foreach(QFileInfo efi, fil)
    {
        auto filI = QDir(efi.absoluteFilePath()).entryInfoList(QStringList() << "*.qml");
        foreach (QFileInfo qmlFI, filI)
        if (searchForQUsb2snesEntry(qmlFI.absoluteFilePath()))
        {
            QString wTitle = searchWindowTitle(qmlFI.absoluteFilePath());
            if (wTitle.isEmpty())
                magic2SnesMenu->addAction(qmlFI.fileName())->setData(qmlFI.absoluteFilePath());
            else
                magic2SnesMenu->addAction(wTitle)->setData(qmlFI.absoluteFilePath());
        }
    }
}


void AppUi::handleMagic2Snes(QString path)
{
    magic2SnesMenu = menu->addMenu(QIcon(":/img/magic2snesicon.png"), "Magic2Snes");
    magic2SnesExe = path + "/Magic2Snes";
    magic2SnesMenu->addAction(QIcon(":/img/magic2snesicon.png"), "Run Magic2Snes");
    magic2SnesMenu->addSeparator();
    connect(magic2SnesMenu, SIGNAL(triggered(QAction*)), this, SLOT(onMagic2SnesMenuTriggered(QAction*)));
    QFileInfo fi(path);
    QString examplePath = fi.absoluteFilePath() + "/examples";
    QString scriptPath = fi.absoluteFilePath() + "/scripts";
    if (QFile::exists(examplePath))
        addMagic2SnesFolder(examplePath);
    if (QFile::exists(scriptPath))
        addMagic2SnesFolder(scriptPath);

}
