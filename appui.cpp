


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
#include <QTranslator>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QVBoxLayout>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)
#define sInfo() qCInfo(log_appUi)

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
    retroarchFactory = nullptr;
    sd2snesFactory = new SD2SnesFactory();
    wsServer.addDeviceFactory(sd2snesFactory);
    QObject::connect(&wsServer, &WSServer::listenFailed, [=](const QString& err) {
        QMessageBox::critical(nullptr, tr("Error starting the websocket server"),
                              QString(tr("There was an error starting the core of the application : %1.\n"
                                         "Make sure you don't have another software running that use the same port (8080). This can be the old Usb2Snes application or Crowd Control for example.")).arg(err));
        qApp->exit(1);
    });
    luaBridge = nullptr;
    snesClassic = nullptr;
    dlManager = nullptr;

    QTranslator translator;
    QString locale = QLocale::system().name().split('_').first();
    translator.load(qApp->applicationDirPath() + "/i18n/qusb2snes_" + locale + ".qm");
    qApp->installTranslator(&translator);

    // Main menu creation is here
    menu = new QMenu();
    menu->addAction(QIcon(":/img/icon64x64.ico"), "QUsb2Snes v" + QApplication::applicationVersion());
    connect(menu->addAction("ESA Soon, Save the children!"), &QAction::triggered, [=]()
    {
        QDesktopServices::openUrl(QUrl("https://esamarathon.com/schedule"));
    });
    menu->addSeparator();

    deviceMenu = menu->addMenu(QIcon(":/img/deviceicon.svg"), tr("Devices", "Menu entry"));
    connect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
    connect(deviceMenu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToshow()));

    appsMenu = menu->addMenu(QIcon(":/img/appicon.svg"), tr("Applications", "Menu entry"));
    appsMenu->addAction("No applications");

    sysTray->setContextMenu(menu);
    QTimer::singleShot(50, this, &AppUi::init);
}

void AppUi::init()
{
    retroarchAction = new QAction(QIcon(":/img/retroarch.png"), tr("Enable RetroArch virtual device (use bsnes-mercury if possible)"));
    retroarchAction->setCheckable(true);
    connect(retroarchAction, SIGNAL(triggered(bool)), this, SLOT(onRetroarchTriggered(bool)));

    luaBridgeAction = new QAction(QIcon(":/img/icone-snes9x.gif"), tr("Enable Lua bridge (Snes9x-rr/BizHawk)"));
    luaBridgeAction->setCheckable(true);
    connect(luaBridgeAction, SIGNAL(triggered(bool)), this, SLOT(onLuaBridgeTriggered(bool)));

    snesClassicAction = new QAction(QIcon(":/img/chrysalis.png"), tr("Enable SNES Classic support (experimental)"));
    snesClassicAction->setCheckable(true);
    connect(snesClassicAction, SIGNAL(triggered(bool)), this, SLOT(onSNESClassicTriggered(bool)));
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
    if (QFile::exists(qApp->applicationDirPath() + "/Magic2Snes") && globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes(qApp->applicationDirPath() + "/Magic2Snes");
    if (!QFile::exists(qApp->applicationDirPath() + "/Magic2Snes") && globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes("");
    if (!globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes(globalSettings->value("Magic2SnesLocation").toString());
    menu->addSeparator();

    miscMenu = menu->addMenu(tr("Misc", "Menu entry"));

#ifdef Q_OS_WIN
    QObject::connect(miscMenu->addAction(QIcon(":/img/updateicon.svg"), tr( "Check for Update")), &QAction::triggered, [this] {
        this->checkForNewVersion(true);
    });
    miscMenu->addSeparator();
    QObject::connect(miscMenu->addAction(QIcon(":/img/microsoft-windows-logo.svg"), tr("Add a 'Send To' entry in the Windows menu")),
                     &QAction::triggered, this, &AppUi::addWindowsSendToEntry);
    if (!globalSettings->contains("SendToSet"))
    {
        QMessageBox msg;
        msg.setText(tr("Do you want to create a entry in the 'Send To' menu of Windows to upload file to the sd2snes?"));
        msg.setInformativeText(tr("This will only be asked once, if you want to add it later go into the Misc menu."));
        msg.setWindowTitle("QUsb2Snes");
        msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Ok);
        int ret = msg.exec();
        if (ret == QMessageBox::Ok)
            addWindowsSendToEntry();
        globalSettings->setValue("SendToSet", true);
    }
    if (!globalSettings->contains("checkUpdateCounter") || globalSettings->value("checkUpdateCounter").toInt() == 5)
    {
        globalSettings->setValue("checkUpdateCounter", 0);
        checkForNewVersion();
    } else {
        globalSettings->setValue("checkUpdateCounter", globalSettings->value("checkUpdateCounter").toInt() + 1);
    }
#endif

    miscMenu->setToolTipsVisible(true);
    QAction* debugLogAction = miscMenu->addAction(tr("Enable debug logs"));
    debugLogAction->setCheckable(true);
    debugLogAction->setChecked(globalSettings->value("debugLog").toBool());
    debugLogAction->setToolTip(tr("Enable the creation of a log file with lot of debug informations"));
    QObject::connect(debugLogAction, &QAction::changed, [=]() {
        if (debugLogAction->isChecked())
        {
            QMessageBox msg;
            msg.setText(tr("Are you sure you want to enable debug log? The file generated could easily go over hundred of MBytes"));
            msg.setInformativeText(tr("You will need to restart QUsb2Snes for this setting to take effect"));
            msg.setWindowTitle("QUsb2Snes");
            msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msg.setDefaultButton(QMessageBox::Ok);
            int ret = msg.exec();
            if (ret == QMessageBox::Ok)
            {
                sInfo() << "Debug log enabled";
                globalSettings->setValue("debugLog", true);
            }
            else {
                debugLogAction->setChecked(false);
            }
        } else {
            sInfo() << "Debug log disabled";
            globalSettings->setValue("debugLog", false);
        }
    });
    QObject::connect(menu->addAction(tr("Exit")), &QAction::triggered, qApp, &QApplication::exit);
    appsMenu->addSeparator();
    appsMenu->addAction(tr("Remote Applications"));
    appsMenu->addSeparator();
    appsMenu->addAction(QIcon(":/img/multitroid.png"), "Multitroid");
    appsMenu->addAction(QIcon(":/img/BK small.png"), "SMZ3 Multiworld");

    connect(qApp, &QCoreApplication::aboutToQuit, [=]() {
      sysTray->hide();
    });
}

void AppUi::updated(QString fromVersion)
{
    QMessageBox::about(nullptr, tr("QUsb2Snes updated succesfully"),
                       QString(tr("QUsb2Snes successfully updated from %1 to version %2")).arg(fromVersion).arg(qApp->applicationVersion()));
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
        QDesktopServices::openUrl(QUrl("http://multitroid.com/"));
    if (action->text() == "SMZ3 Multiworld")
        QDesktopServices::openUrl(QUrl("https://multiworld.samus.link"));
    if (action->data().isNull())
        return ;
    const ApplicationInfo& appInfo = regularApps[action->data().toString()];
    QProcess proc(this);
    //proc.setWorkingDirectory(fi.path());
    QString exec;
    QString wDir = appInfo.folder;
    QStringList arg;
    if (appInfo.isQtApp)
    {
        wDir = qApp->applicationDirPath();
#ifdef Q_OS_WIN
        arg << "-platformpluginpath" << qApp->applicationDirPath() + "/platforms/";
#endif
    }
#ifdef Q_OS_WIN
    exec = appInfo.folder + "/" + appInfo.executable + ".exe";
#else
    exec = appInfo.folder + "/" + appInfo.executable;
#endif
    bool ok = proc.startDetached(exec, arg, wDir);
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

void AppUi::checkForNewVersion(bool manual)
{
    sInfo() << "Checking for new version - SSL support is : " << QSslSocket::supportsSsl() << QSslSocket::sslLibraryBuildVersionString();
    if (!QSslSocket::supportsSsl())
        return;
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply* reply)
    {
        sDebug() << "Finished" << reply->size();
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jArr = doc.array();
        QString lastTag = jArr.at(0).toObject().value("tag_name").toString();
        QString body = jArr.at(0).toObject().value("body").toString();
        QString name = jArr.at(0).toObject().value("name").toString();
        sInfo() << "Latest release is " << lastTag;
        qDebug() << body;
        body.replace('\n', "<br/>");
        body.replace('\r', "");

        if (QVersionNumber::fromString(qApp->applicationVersion()) < QVersionNumber::fromString(lastTag.remove(0, 1)))
        {
            QMessageBox msg;
            msg.setText(QString(tr("A new version of QUsb2Snes is available : QUsb2Snes %1\nDo you want to upgrade to it?")).arg(lastTag));
            msg.setWindowTitle(tr("New version of QUsb2Snes available"));
            msg.setInformativeText(QString("<b>%1</b><p>%2</p>").arg(name).arg(body));
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::No);
            msg.setDefaultButton(QMessageBox::No);
            int but = msg.exec();
            if (but == QMessageBox::Yes)
            {
                if (dlManager == nullptr)
                {
                    dlManager = new QNetworkAccessManager();
                    dlManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
                    QObject::connect(dlManager, &QNetworkAccessManager::finished, this, &AppUi::DLManagerRequestFinished);
                    dlLabel = new QLabel();
                    dlProgressBar = new QProgressBar();
                    dlProgressBar->setTextVisible(false);
                    dlLabel->setText(tr("Downloading the Windows Updater"));
                    dlWindow = new QWidget();
                    QVBoxLayout* layout = new QVBoxLayout();
                    dlWindow->setWindowTitle("Updating Updater");
                    dlWindow->setWindowIcon(QIcon(":/icon64x64.ico"));
                    dlWindow->setLayout(layout);
                    layout->addWidget(dlLabel);
                    layout->addWidget(dlProgressBar);
                }
                dlWindow->show();
                dlManager->get(QNetworkRequest(QUrl("https://api.github.com/repos/Skarsnik/QUsb2Snes/releases")));
                //QString upExe(qApp->applicationDirPath() + "/WinUpdater.exe");
                //(int)::ShellExecute(0, reinterpret_cast<const WCHAR*>("runas"), reinterpret_cast<const WCHAR*>(upExe.utf16()), 0, 0, SW_SHOWNORMAL);
                //QProcess::startDetached(upExe);
                //qApp->exit(0);
            }
        } else {
            if (manual)
                QMessageBox::information(nullptr, tr("No new version of QUsb2Snes available"), tr("No new version of QUsb2Snes available"));
        }
    });
    manager->get(QNetworkRequest(QUrl("https://api.github.com/repos/Skarsnik/QUsb2snes/releases")));
    sDebug() << "Get";
}

void AppUi::checkForApplications()
{
    QDir    appsDir(qApp->applicationDirPath() + "/apps");
    if (!appsDir.exists())
        return ;
    foreach (QFileInfo fi, appsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        sInfo() << "Scanning " << fi.absoluteFilePath();
        ApplicationInfo appInfo;
        sDebug() << "Searching for " << fi.absoluteFilePath() + "/" + applicationJsonFileName;
        if (QFile::exists(fi.absoluteFilePath() + "/" + applicationJsonFileName))
        {
            qDebug() << "Found a json description file "  + applicationJsonFileName;
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
        {
            regularApps[appInfo.folder] = appInfo;
            sInfo() << "Found an application" << appInfo;
        }
    }
    if (regularApps.isEmpty())
        return ;
    appsMenu->clear();
    appsMenu->addAction(tr("Local Applications"));
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
    bool ok = QFile::link(qApp->applicationDirPath() + "/SendToSd2Snes.exe", appData.path() + "/SD2Snes.lnk");
    QMessageBox msg;
    if (ok)
    {
        msg.setText(tr("Entry in the Send To menu has been added successfully."));
        sInfo() << "Entry in the Send to menu added";
    } else {
        msg.setText(QString(tr("Error while creating the Send To entry.<br>Check in %1 if it does not already exist.")).arg(appData.path()));
    }
    msg.exec();
}

void AppUi::onUntrustedConnection(QString origin)
{
    disconnect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
    QMessageBox msg;
    msg.setText(QString(tr("Received a connection from an untrusted origin %1. Do you want to add this source to the trusted origin list?")).arg(origin));
    msg.setWindowTitle(tr("Untrusted origin"));
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    int but = msg.exec();
    if (but == QMessageBox::Yes)
    {
        QStringList tList = globalSettings->value("trustedOrigin").toString().split(";");
        wsServer.addTrusted(origin);
        tList.append(origin);
        globalSettings->setValue("trustedOrigin", tList.join(";"));
    }
    connect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
}

QDebug operator<<(QDebug debug, const AppUi::ApplicationInfo &req)
{
    debug << "Name : " << req.name << " Description : " << req.description
          << "Folder : " << req.folder << "Icon : " <<req.icon << "Exe :" << req.executable
          << "QtApp :" << req.isQtApp;
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


// TODO, probably need to rework the whole device status thing again

void    AppUi::addDevicesInfo(DeviceFactory* devFact)
{
    QString statusString;

    QList<ADevice*> devs = devFact->getDevices();
    if (devs.isEmpty())
    {
        statusString = QString("%1 : %2").arg(devFact->name(), -20).arg(devFact->status());
    } else {
        foreach (ADevice* dev, devs)
        {
            statusString = dev->name();
            QStringList clients = wsServer.getClientsName(dev);
            if (clients.isEmpty())
            {
                if (dev->state() == ADevice::READY)
                    statusString += tr(" - No client connected");
                if (dev->state() == ADevice::CLOSED)
                    statusString += tr("Device not ready : ") + devFact->status();
            } else {
                statusString += " : " + clients.join(" - ");
            }
        }
    }
    deviceMenu->addAction(statusString);
}


// Code to download the updater

void    AppUi::DLManagerRequestFinished(QNetworkReply* reply)
{
    static int step = 0;

    QByteArray data = reply->readAll();
    qDebug() << reply->error();
    if (step == 0)
    {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jArr = doc.array();
        QJsonArray assets = jArr.at(0).toObject().value("assets").toArray();
        foreach(const QJsonValue& value, assets)
        {
            if (value.toObject().value("name").toString() == "WinUpdater.exe")
            {
                sDebug() << "Downloading " << value.toObject().value("browser_download_url").toString();
                dlLabel->setText(QString(QObject::tr("Downloading new WinUpdater %1")).arg(
                                  jArr.at(0).toObject().value("tag_name").toString()));
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                QNetworkReply*  dlReply = dlManager->get(req);
                QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
                   sDebug() << "DL reply redirected";
                });
                QObject::connect(dlReply, &QNetworkReply::downloadProgress, [=](qint64 bytesRcv, qint64 bytesTotal)
                {
                    qDebug() << 20 + (bytesRcv / bytesTotal) * 80;
                    dlProgressBar->setValue(20 + (bytesRcv / bytesTotal) * 80);
                });
                sDebug() << "Found WinUpdater.exe asset";
                step = 1;
                return ;
            }
        }
        qApp->exit(1);
     }
     if (step == 1)
     {
         QFile file(qApp->applicationDirPath() + "/WinUpdater.exe");

         step = 0;
         dlProgressBar->setValue(100);
         dlLabel->setText(QObject::tr("Writing WinUpdater.exe"));
         if (file.open(QIODevice::WriteOnly))
         {
            sDebug() << data.size();
            file.write(data);
            sDebug() << "WinUpdater written";
            file.flush();
            file.close();
            dlLabel->setText(QObject::tr("Starting the Windows Updater"));
            QProcess::startDetached(qApp->applicationDirPath() + "/WinUpdater.exe");
            QTimer::singleShot(200, [=] {qApp->exit(0);});
         } else {
             dlWindow->hide();
             QMessageBox::warning(nullptr, tr("Error downloading the updater"), tr("There was an error downloading or writing the updater file, please try to download it manually on the release page"));
         }
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
    if (path.isEmpty())
    {
        magic2SnesExe = "";
        magic2SnesMenu->addAction(tr("Could not find Magic2Snes"));
        magic2SnesMenu->addAction(tr("Click here to set the path to it"));
        return ;
    }
    magic2SnesExe = path + "/Magic2Snes";
    magic2SnesMenu->addAction(QIcon(":/img/magic2snesicon.png"), tr("Run Magic2Snes"));
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

