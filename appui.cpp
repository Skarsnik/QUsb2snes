#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QStyle>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)

#include "appui.h"
#include "devices/snesclassic.h"
#include "wsserver.h"



extern WSServer    wsServer;

AppUi::AppUi(QObject *parent) : QObject(parent)
{
    sysTray = new QSystemTrayIcon(QIcon(":/img/icon64x64.ico"));
    qApp->setQuitOnLastWindowClosed(false);
    menu = new QMenu();
    menu->addAction(QIcon(":/img/icon64x64.ico"), "QUsb2Snes v" + QApplication::applicationVersion());
    menu->addSeparator();
    retroarchDevice = nullptr;
    sd2snesFactory = new SD2SnesFactory();
    wsServer.addDeviceFactory(sd2snesFactory);
    luaBridge = nullptr;
    snesClassic = nullptr;

    deviceMenu = menu->addMenu(QIcon(":/img/deviceicon.svg"), "Devices");
    connect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
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
        luaBridge = new LuaBridge();
        wsServer.addDeviceFactory(luaBridge);
        luaBridgeAction->setChecked(true);
    }
    if (settings->value("snesclassic").toBool())
    {
        snesClassic = new SNESClassicFactory();
        wsServer.addDeviceFactory(snesClassic);
        snesClassicAction->setChecked(true);
    }
    if (settings->contains("trustedOrigin"))
    {
        sDebug() << settings->value("trustedOrigin").toString().split(";");
        foreach (QString ori, settings->value("trustedOrigin").toString().split(";"))
        {
            wsServer.addTrusted(ori);
        }
    }
    checkForApplications();
    menu->addSeparator();
    handleMagic2Snes(qApp->applicationDirPath() + "/Magic2Snes");
    menu->addSeparator();

    miscMenu = menu->addMenu("Misc");
#ifdef Q_OS_WIN
    QObject::connect(miscMenu->addAction("Add a Send To entry in the Windows menu"),
                     &QAction::triggered, this, &AppUi::addWindowsSendToEntry);
    QSettings settings("skarsnik.nyo.fr", "QUsb2Snes");
    if (!settings.contains("SendToSet"))
    {
        QMessageBox msg;
        msg.setText(tr("Do you want to create a entry in the Send To menu of Windows to upload file to the sd2snes?"));
        msg.setInformativeText("This will only be asked once, if you want to add it later go into the Misc menu.");
        msg.setWindowTitle("QUsb2Snes");
        msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Ok);
        int ret = msg.exec();
        if (ret == QMessageBox::Ok)
            addWindowsSendToEntry();
        settings.setValue("SendToSet", true);
    }
#endif

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
        if (luaBridge == nullptr)
            luaBridge = new LuaBridge();
        wsServer.addDeviceFactory(luaBridge);
    } else {
        //wsServer.removeDevice(luaBridgeDevice);
    }
    settings->setValue("luabridge", checked);
}

void AppUi::onSNESClassicTriggered(bool checked)
{
    if (checked == true)
    {
        if (snesClassic == nullptr)
            snesClassic = new SNESClassicFactory();
        wsServer.addDeviceFactory(snesClassic);
    } else {
        //wsServer.removeDevice(snesClassic);
    }
    settings->setValue("snesclassic", checked);
}

void    AppUi::addDevicesInfo(DeviceFactory* devFact)
{
    QString statusString;

    QList<ADevice*> devs = devFact->getDevices();
    if (devs.isEmpty())
    {
        statusString = devFact->status();
    } else {
        foreach (ADevice* dev, devs)
        {
            statusString = dev->name();
            QStringList clients = wsServer.getClientsName(dev);
            if (clients.isEmpty())
                statusString += " - No client connected";
            else
                statusString += " : " + clients.join(" - ");
        }
    }
    deviceMenu->addAction(statusString);
}

void AppUi::onMenuAboutToshow()
{
    deviceMenu->clear();
    deviceMenu->addAction("Devices state");
    deviceMenu->addSeparator();
    if (sd2snesFactory != nullptr)
        addDevicesInfo(sd2snesFactory);
    if (luaBridge != nullptr)
        addDevicesInfo(luaBridge);
    if (snesClassic != nullptr)
        addDevicesInfo(snesClassic);
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
    QFileInfo fi(action->data().toString());
    QProcess proc(this);
    //proc.setWorkingDirectory(fi.path());
    bool ok = proc.startDetached(fi.absoluteFilePath(), QStringList(), fi.path());
    sDebug() << "Running " << fi.absoluteFilePath() << " in " << fi.path() << ok;
    if (!ok)
        sDebug() << "Error running " << fi.absoluteFilePath() << proc.errorString();
}

void AppUi::checkForApplications()
{
    QDir    appsDir(qApp->applicationDirPath() + "/apps");
    if (!appsDir.exists())
        return ;
    foreach (QFileInfo fi, appsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        sDebug() << "Scanning " << fi.absoluteFilePath();
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


void AppUi::addWindowsSendToEntry()
{
    QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    appData.cdUp();
    appData.cd("Microsoft");
    appData.cd("Windows");
    appData.cd("SendTo");
    bool ok = QFile::link(qApp->applicationDirPath() + "/apps/QFile2Snes/QFile2Snes.exe", appData.path() + "/SD2Snes.lnk");
    QMessageBox msg;
    if (ok)
        msg.setText(tr("Entry in the Send To menu has been added successfully"));
    else
        msg.setText(QString(tr("Error while creating the Send To entry.<br>Check in %1 if it does not already exist")).arg(appData.path()));
    msg.exec();
}

void AppUi::onUntrustedConnection(QString origin)
{
    QMessageBox msg;
    msg.setText(QString(tr("Received a connection from an untrusted origin %1. Do you want to add this source to the trusted origin list")).arg(origin));
    msg.setWindowTitle(tr("Untrusted origin"));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    int but = msg.exec();
    sDebug() << but;
    if (but == QMessageBox::Yes)
    {
        wsServer.addTrusted(origin);
        QStringList tList = settings->value("trustedOrigin").toString().split(";");
        tList.append(origin);
        settings->setValue("trustedOrigin", tList.join(";"));
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
