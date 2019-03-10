#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QStyle>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)

#include "appui.h"
#include "devices/snesclassic.h"
#include "wsserver.h"

const  QString             applicationJsonFileName = "qusb2snesapp.json";

extern WSServer            wsServer;
extern QSettings*          globalSettings;

AppUi::AppUi(QObject *parent) : QObject(parent)
{
    sysTray = new QSystemTrayIcon(QIcon(":/img/icon64x64.ico"));
    qApp->setQuitOnLastWindowClosed(false);
    menu = new QMenu();
    menu->addAction(QIcon(":/img/icon64x64.ico"), "QUsb2Snes v" + QApplication::applicationVersion());
    menu->addSeparator();
    retroarchFactory = nullptr;
    sd2snesFactory = new SD2SnesFactory();
    wsServer.addDeviceFactory(sd2snesFactory);
    luaBridge = nullptr;
    snesClassic = nullptr;

    deviceMenu = menu->addMenu(QIcon(":/img/deviceicon.svg"), "Devices");
    connect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
    connect(deviceMenu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToshow()));


    retroarchAction = new QAction(QIcon(":/img/retroarch.png"), "Enable RetroArch virtual device (use bsnes-mercury if possible)");
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
    if (globalSettings->value("retroarchdevice").toBool())
    {
        retroarchFactory = new RetroArchFactory();
        wsServer.addDeviceFactory(retroarchFactory);
        retroarchAction->setChecked(true);
    }
    if (globalSettings->value("luabridge").toBool())
    {
        luaBridge = new LuaBridge();
        wsServer.addDeviceFactory(luaBridge);
        luaBridgeAction->setChecked(true);
    }
    if (globalSettings->value("snesclassic").toBool())
    {
        snesClassic = new SNESClassicFactory();
        wsServer.addDeviceFactory(snesClassic);
        snesClassicAction->setChecked(true);
    }
    if (globalSettings->contains("trustedOrigin"))
    {
        sDebug() << globalSettings->value("trustedOrigin").toString().split(";");
        foreach (QString ori, globalSettings->value("trustedOrigin").toString().split(";"))
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
    if (!globalSettings->contains("SendToSet"))
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
        globalSettings->setValue("SendToSet", true);
    }
#endif

    QObject::connect(menu->addAction("Exit"), &QAction::triggered, qApp, &QApplication::exit);
    appsMenu->addSeparator();
    appsMenu->addAction("Remote Applications");
    appsMenu->addSeparator();
    appsMenu->addAction(QIcon(":/img/multitroid.png"), "Multitroid");

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
    if (retroarchFactory != nullptr)
        addDevicesInfo(retroarchFactory);
    deviceMenu->addSeparator();
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addAction(snesClassicAction);
}

void AppUi::onAppsMenuTriggered(QAction *action)
{
    if (action->text() == "Multitroid")
    {
        QDesktopServices::openUrl(QUrl("http://multitroid.com/"));
        /*QWebEngineView *view = new QWebEngineView(parent);
        view->load(QUrl("http://qt-project.org/"));
        view->show();*/
    }
    if (action->data().isNull())
        return ;
    const ApplicationInfo& appInfo = regularApps[action->data().toString()];
    QProcess proc(this);
    //proc.setWorkingDirectory(fi.path());
    QString exec;
    QString wDir = appInfo.folder;
    if (appInfo.isQtApp)
        wDir = qApp->applicationDirPath();
#ifdef Q_OS_WIN
    exec = appInfo.folder + "/" + appInfo.executable + ".exe";
#else
    exec = appInfo.folder + "/" + appInfo.executable;
#endif
    bool ok = proc.startDetached(exec, QStringList(), wDir);
    sDebug() << "Running " << exec << " in " << wDir << ok;
    if (!ok)
        sDebug() << "Error running " << exec << proc.errorString();
}

AppUi::ApplicationInfo AppUi::parseJsonAppInfo(QString fileName)
{
    ApplicationInfo info;
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QJsonDocument   jdoc = QJsonDocument::fromJson(file.readAll());
    QJsonObject job = jdoc.object();
    if (job.contains("QtApp"))
        info.isQtApp = job["QtApp"].toBool();
    info.name = job["name"].toString();
    info.folder = QFileInfo(fileName).absolutePath();
    info.description = job["description"].toString();
    info.executable = job["executable"].toString();
    info.icon = job["icon"].toString();
    return info;
}

void AppUi::checkForApplications()
{
    QDir    appsDir(qApp->applicationDirPath() + "/apps");
    if (!appsDir.exists())
        return ;
    foreach (QFileInfo fi, appsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        sDebug() << "Scanning " << fi.absoluteFilePath();
        ApplicationInfo appInfo;
        sDebug() << "Searching for " << fi.absoluteFilePath() + "/" + applicationJsonFileName;
        if (QFile::exists(fi.absoluteFilePath() + "/" + applicationJsonFileName))
        {
            qDebug() << "Found a json file "  + applicationJsonFileName;
            appInfo = parseJsonAppInfo(fi.absoluteFilePath() + "/" + applicationJsonFileName);
        } else {
            QDir dir(fi.absoluteFilePath());
            QFileInfoList fil = dir.entryInfoList(QDir::Files | QDir::Executable);
            if (!fil.isEmpty())
            {
                appInfo.name = fi.baseName();
                appInfo.executable = fil.first().baseName();
                appInfo.folder = fi.absoluteFilePath();
                if (QFile::exists(fi.absoluteFilePath() + "/icone.png"))
                    appInfo.icon = "icone.png";
            }
        }
        if (!appInfo.name.isEmpty())
            regularApps[appInfo.folder] = appInfo;
    }
    if (regularApps.isEmpty())
        return ;
    appsMenu->clear();
    appsMenu->addAction("Local Applications");
    appsMenu->addSeparator();
    appsMenu->setToolTipsVisible(true);
    connect(appsMenu, SIGNAL(triggered(QAction*)), this, SLOT(onAppsMenuTriggered(QAction*)));
    QMapIterator<QString, ApplicationInfo> it(regularApps);
    while (it.hasNext())
    {
        it.next();
        ApplicationInfo info = it.value();
        sDebug() << "Adding " << it.key() << " - " << info.name;
        sDebug() << info;
        QAction *act;
        if (!it.value().icon.isEmpty())
            act = appsMenu->addAction(QIcon(info.folder + "/" + info.icon), info.name);
        else
            act = appsMenu->addAction(info.name);
        act->setData(it.key());
        if (!info.description.isEmpty())
            act->setToolTip(info.description);
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
        QStringList tList = globalSettings->value("trustedOrigin").toString().split(";");
        tList.append(origin);
        globalSettings->setValue("trustedOrigin", tList.join(";"));
    }
}

QDebug operator<<(QDebug debug, const AppUi::ApplicationInfo &req)
{
    debug << req.name << " Description : " <<req.description << req.folder << req.icon << req.executable << req.isQtApp << "\n";
    return debug;
}



void AppUi::onRetroarchTriggered(bool checked)
{
    if (checked == true)
    {
        if (retroarchFactory == nullptr)
            retroarchFactory = new RetroArchFactory();
        wsServer.addDeviceFactory(retroarchFactory);
    } else {
        //wsServer.removeDevice(retroarchFactory);
    }
    globalSettings->setValue("retroarchdevice", checked);
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
    globalSettings->setValue("luabridge", checked);
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
    globalSettings->setValue("snesclassic", checked);
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
