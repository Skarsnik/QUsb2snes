/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
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
#include <QSettings>
#include <QScreen>

#include <ui/wizard/devicesetupwizard.h>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)
#define sInfo() qCInfo(log_appUi)

#include "../usb2snes.h"
#include "appui.h"
#include "../wsserver.h"
#include "sqpath.h"


static const  QString       applicationJsonFileName = "qusb2snesapp.json";
extern QSettings*           globalSettings;
extern WSServer             wsServer;


AppUi::AppUi(QObject *parent) : QObject(parent)
{
    sysTray = new QSystemTrayIcon(QIcon(":/img/icon64x64.ico"));
    qApp->setQuitOnLastWindowClosed(false);
    checkingDeviceInfos = false;

    retroarchFactory = nullptr;
    sd2snesFactory = nullptr;
    luaBridge = nullptr;
    snesClassic = nullptr;
    emuNWAccess = nullptr;
    dlManager = nullptr;

    QTranslator translator;
    QString locale = QLocale::system().name().split('_').first();
    translator.load(qApp->applicationDirPath() + "/i18n/qusb2snes_" + locale + ".qm");
    qApp->installTranslator(&translator);

    // Main menu creation is here
    menu = new QMenu();
    menu->addAction(QIcon(":/img/icon64x64.ico"), "QUsb2Snes v" + QApplication::applicationVersion());
    connect(menu->addAction("Joins us on Discord!"), &QAction::triggered, [=]()
    {
        QDesktopServices::openUrl(QUrl("https://discord.gg/2JgefTX"));
    });
    menu->addSeparator();

    deviceMenu = menu->addMenu(QIcon(":/img/deviceicon.svg"), tr("Devices", "Menu entry"));
    connect(&wsServer, &WSServer::untrustedConnection, this, &AppUi::onUntrustedConnection);
    connect(&wsServer, &WSServer::deviceFactoryStatusDone, this, &AppUi::onDeviceFactoryStatusDone);
    connect(&wsServer, &WSServer::newDeviceFactoryStatus, this, &AppUi::onDeviceFactoryStatusReceived);
#ifdef Q_OS_LINUX
    connect(deviceMenu, &QMenu::aboutToShow, this, [=]{
        onMenuHovered(deviceMenu->menuAction());
        ;});
#else
    connect(menu, &QMenu::hovered, this, &AppUi::onMenuHovered);
#endif
    appsMenu = menu->addMenu(QIcon(":/img/appicon.svg"), tr("Applications", "Menu entry"));
    appsMenu->addAction("No applications");

    QTimer::singleShot(50, this, &AppUi::init);
}

void AppUi::init()
{
    sd2snesAction = new QAction(QIcon(":/img/ikari.ico"), tr("Enable SD2SNES/FxPak pro Support"));
    sd2snesAction->setCheckable(true);
    connect(sd2snesAction, &QAction::triggered, this, &AppUi::onSD2SnesTriggered);

    retroarchAction = new QAction(QIcon(":/img/retroarch.png"), tr("Enable RetroArch virtual device (use bsnes-mercury if possible)"));
    retroarchAction->setCheckable(true);
    connect(retroarchAction, &QAction::triggered, this, &AppUi::onRetroarchTriggered);

    luaBridgeAction = new QAction(QIcon(":/img/icone-snes9x.gif"), tr("Enable Lua bridge - DEPRECATED"));
    luaBridgeAction->setCheckable(true);
    connect(luaBridgeAction, &QAction::triggered, this, &AppUi::onLuaBridgeTriggered);

    snesClassicAction = new QAction(QIcon(":/img/chrysalis.png"), tr("Enable SNES Classic support"));
    snesClassicAction->setCheckable(true);
    connect(snesClassicAction, &QAction::triggered, this, &AppUi::onSNESClassicTriggered);

    emuNWAccessAction = new QAction(QIcon(":/img/mr chip.png"), tr("Enable EmuNetworkAccess"));
    emuNWAccessAction->setCheckable(true);
    connect(emuNWAccessAction, &QAction::triggered, this, &AppUi::onEmuNWAccessTriggered);

    if (globalSettings->value("sd2snessupport").toBool())
    {
        addSD2SnesFactory();
        sd2snesAction->setChecked(true);
    }
    if (globalSettings->value("retroarchdevice").toBool())
    {
        addRetroarchFactory();
        retroarchAction->setChecked(true);
    }
    if (globalSettings->value("luabridge").toBool())
    {
        addLuaBridgeFactory();
        luaBridgeAction->setChecked(true);
    }
    if (globalSettings->value("snesclassic").toBool())
    {
        addSnesClassicFactory();
        snesClassicAction->setChecked(true);
    }
    if (globalSettings->value("emunwaccess").toBool())
    {
        addNWAFactory();
        emuNWAccessAction->setChecked(true);
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
    connect(qApp, &QCoreApplication::aboutToQuit, this, [=]() {
      sysTray->hide();
    });
    if (!globalSettings->contains("FirstTime"))
    {
        sDebug() << "Starting Wizzard";
        globalSettings->setValue("FirstTime", true);
        DeviceSetupWizard wiz;
        if (wiz.exec() == QDialog::Accepted)
        {
            if (wiz.deviceSelected() == DeviceSetupWizard::SD2SNES)
            {
                onSD2SnesTriggered(true);
                if (wiz.contextMenu())
                    addWindowsSendToEntry();
            }
            if (wiz.deviceSelected() == DeviceSetupWizard::RETROARCH)
            {
                retroarchAction->setChecked(true);
                onRetroarchTriggered(true);
            }
            if (wiz.deviceSelected() == DeviceSetupWizard::SNESCLASSIC)
            {
                snesClassicAction->setChecked(true);
                onSNESClassicTriggered(true);
            }
            if (wiz.deviceSelected() == DeviceSetupWizard::NWA)
            {
                emuNWAccessAction->setChecked(true);
                onEmuNWAccessTriggered(true);
            }
            if (wiz.sendTelemetry())
            {
                postTelemetryData();
            }
            globalSettings->setValue("telemetrydatahandled", true);
        }
    }
    setMenu();
    sysTray->setContextMenu(menu);
    UiWidget = new SysTrayWidget();
    UiWidget->contextMenu = menu;

    connect(sysTray, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
        {
            if (UiWidget->isVisible() == false)
            {
                QRect geo = sysTray->geometry();
                sDebug() << "Systray geo" << geo;
                QPoint tray_center   = sysTray->geometry().center();
                if (tray_center == QPoint(0, 0))
                {
                    QPoint mousePos = QCursor::pos() - qApp->screenAt(QCursor::pos())->geometry().topLeft();
                    geo = QRect(mousePos, QSize(10, 10));
                    tray_center = mousePos;
                }
                QRect  screen_rect   = qApp->screenAt(tray_center)->geometry();
                QPoint screen_center = screen_rect.center();

                Qt::Corner corner = Qt::TopLeftCorner;
                sDebug() << tray_center << screen_center << UiWidget->size();
                if (tray_center.x() > screen_center.x() && tray_center.y() <= screen_center.y())
                    corner = Qt::TopRightCorner;
                else if (tray_center.x() > screen_center.x() && tray_center.y() > screen_center.y())
                    corner = Qt::BottomRightCorner;
                else if (tray_center.x() <= screen_center.x() && tray_center.y() > screen_center.y())
                    corner = Qt::BottomLeftCorner;
                // Bottom
                sDebug() << "Systray is in : " << corner;
                if (corner == Qt::BottomRightCorner)
                    UiWidget->move(geo.x() - UiWidget->size().width(), geo.y() - UiWidget->size().height());
                if (corner == Qt::BottomLeftCorner)
                    UiWidget->move(geo.x() + UiWidget->size().width(), geo.y() - UiWidget->size().height());
                if (corner == Qt::TopLeftCorner)
                    UiWidget->move(geo.x() + UiWidget->size().width(), geo.y() + geo.height());
                if (corner == Qt::TopRightCorner)
                    UiWidget->move(geo.x() - UiWidget->size().width(), geo.y() + geo.height());
                UiWidget->show();

                onMenuHovered(nullptr);
            } else {
                UiWidget->hide();
            }

        }
    });
    startWServer();
}


void   AppUi::startWServer()
{
    quint16 port = 0;
    QHostAddress addr(QHostAddress::Any);
    if (globalSettings->contains("listen"))
        addr = QHostInfo::fromName(globalSettings->value("listen").toString()).addresses().first();
    if (globalSettings->contains("port"))
        port = globalSettings->value("port").toUInt();
    QString status;
    // If the port is changed manually
    if (port != 0)
    {
        status = wsServer.start(addr, port);
        if (!status.isEmpty())
        {
            QMessageBox::critical(nullptr, tr("Error starting the application"), QString(tr("Error starting the core of the application\n Error listening on %1:%2 : %3\nMake sure nothing else is using this port").arg(addr.toString()).arg(port).arg(status)));
            qApp->exit(1);
        }
        return ;
    }
    // Default behavior
    status = wsServer.start(addr, USB2SnesWS::legacyPort);
    if (!status.isEmpty())
    {
        QMessageBox::warning(nullptr, tr("Error listening on legacy port"),
                     QString(tr("There was an error listenning on port 8080 :%1\n"
                                "This can be that QUsb2Snes is already running, the legacy USB2SNES application or something else.\n\n"
                                "Old applications would likely not work.")).arg(status));
    }
    status = wsServer.start(addr, USB2SnesWS::defaultPort);
    if (!status.isEmpty())
    {
        QMessageBox::critical(nullptr, tr("Error listenning on normal port"),
                              QString(tr("There was an error starting the core of the application : %1\n"
                                         "Make sure there is no other Usb2Snes webserver application running (QUsb2Snes/Crowd Control")));
        qApp->exit(1);
    }
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
    if (false)
    {
        QFile desktopFile(job["linux-desktop"].toString());
        if (desktopFile.open(QIODevice::ReadOnly) == false)
        {
            return ApplicationInfo();
        }
        QTextStream fileStream(&desktopFile);
        QString line;
        while (fileStream.readLineInto(&line))
        {
            if (line.startsWith("Icon"))
            {
                info.icon = "/usr/share/";
            }
        }
    }
    info.executable = job["executable"].toString();
    info.icon = job["icon"].toString();
    return info;
}


void AppUi::checkForApplications()
{
    QDir    appsDir(SQPath::softwareDatasPath() + "/apps");
    if (appsDir.exists() == false)
        return ;
    if (appsDir.exists() == true)
    {
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
    }
/*#ifdef SQPROJECT_INSTALLED && SQPROJECT_UNIX_INSTALL_PREFIX
    for (const QFileInfo fi : appsDir.entryInfoList(QDir::NoDotAndDotDot, QDir::Files))
    {
        if (fi.suffix() == "json")
        {
           ApplicationInfo appInfo = parseJsonAppInfo(fi.absoluteFilePath());
           regularApps[appInfo.name] = appInfo;
           sInfo() << "Found an application" << appInfo;
        }
    }
#endif*/
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

void AppUi::addSD2SnesFactory()
{
    if (sd2snesFactory == nullptr)
    {
        sd2snesFactory = new SD2SnesFactory();
        wsServer.addDeviceFactory(sd2snesFactory);
    }
}

void AppUi::onSD2SnesTriggered(bool checked)
{
    if (checked == true)
    {
        addSD2SnesFactory();
    }
    globalSettings->setValue("sd2snessupport", checked);
}

void AppUi::addRetroarchFactory()
{
    if (retroarchFactory == nullptr)
    {
        retroarchFactory = new RetroArchFactory();
        wsServer.addDeviceFactory(retroarchFactory);
    }
}

void AppUi::onRetroarchTriggered(bool checked)
{
    if (checked == true)
    {
        addRetroarchFactory();
    } else {
        //wsServer.removeDevice(retroarchFactory);
    }
    globalSettings->setValue("retroarchdevice", checked);
}

void AppUi::addLuaBridgeFactory()
{
    if (luaBridge == nullptr)
    {
        luaBridge = new LuaBridge();
        wsServer.addDeviceFactory(luaBridge);
    }
}

void AppUi::onLuaBridgeTriggered(bool checked)
{
    if (checked == true)
    {
        addLuaBridgeFactory();
    } else {
        //wsServer.removeDevice(luaBridgeDevice);
    }
    globalSettings->setValue("luabridge", checked);
}

void AppUi::addSnesClassicFactory()
{
    if (snesClassic == nullptr)
    {
        snesClassic = new SNESClassicFactory();
        wsServer.addDeviceFactory(snesClassic);
    }
}

void AppUi::onSNESClassicTriggered(bool checked)
{
    if (checked == true)
    {
        addSnesClassicFactory();
    } else {
        //wsServer.removeDevice(snesClassic);
    }
    globalSettings->setValue("snesclassic", checked);
}

void AppUi::addNWAFactory()
{
    if (emuNWAccess == nullptr)
    {
        emuNWAccess = new EmuNetworkAccessFactory();
        if (!LocalStorage::isUsable())
        {
            if (!QFileInfo::exists(qApp->applicationDirPath() + "/Games"))
                QDir(qApp->applicationDirPath()).mkdir("Games");
            LocalStorage::setRootPath(qApp->applicationDirPath() + "/Games");
        }
        wsServer.addDeviceFactory(emuNWAccess);
    }
}

void AppUi::onEmuNWAccessTriggered(bool checked)
{
    if (checked == true)
    {
        addNWAFactory();
    }
    globalSettings->setValue("emunwaccess", checked);
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
    static QRegularExpression exp("windowTitle\\s*\\:\\s*\"(.+)\"");
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
                auto match = exp.match(line);
                if (match.hasMatch())
                {
                    sDebug() << "Found : " << match.captured(1);
                    return match.captured(1);
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


