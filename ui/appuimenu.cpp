#include <QLoggingCategory>
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include <QStyle>
#include <QFileSystemModel>
#include "appui.h"
#include "sqpath.h"

#include "diagnosticdialog.h"
#include "wsserver.h"

Q_LOGGING_CATEGORY(log_appUiMenu, "APPUIMenu")
#define sDebug() qCDebug(log_appUiMenu)
#define sInfo() qCInfo(log_appUiMenu)


extern QSettings*          globalSettings;
extern WSServer            wsServer;

void    AppUi::setMenu()
{
    menu->addSeparator();
    if (QFile::exists(qApp->applicationDirPath() + "/Magic2Snes") && globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes(qApp->applicationDirPath() + "/Magic2Snes");
    if (!QFile::exists(qApp->applicationDirPath() + "/Magic2Snes") && globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes("");
    if (checkPopTracker())
    {
        addPopTrackerMenu();
        for (const auto& key : regularApps.keys())
        {
            if (regularApps[key].name == "Poptracker")
            {
                regularApps.remove(key);
                sDebug() << "Remove poptracker from app list";
                break;
            }
        }
    }
    createApplicationMenu();
    if (!globalSettings->value("Magic2SnesLocation").toString().isEmpty())
        handleMagic2Snes(globalSettings->value("Magic2SnesLocation").toString());
    menu->addSeparator();

    miscMenu = menu->addMenu(tr("Misc", "Menu entry"));
#ifdef Q_OS_WIN
        bool noUpdate = globalSettings->contains("windowNoUpdate") && globalSettings->value("windowNoUpdate").toBool();
        if (!noUpdate)
        {
            QObject::connect(miscMenu->addAction(QIcon(":/img/updateicon.svg"), tr( "Check for Update")), &QAction::triggered, this, [this] {
                this->checkForNewVersion(true);
            });
        }
        miscMenu->addSeparator();
        QObject::connect(miscMenu->addAction(QIcon(":/img/microsoft-windows-logo.svg"), tr("Add a 'Send To' entry in the Windows menu")),
                         &QAction::triggered, this, &AppUi::addWindowsSendToEntry);
        if (!noUpdate)
        {
            if (!globalSettings->contains("checkUpdateCounter") || globalSettings->value("checkUpdateCounter").toInt() == 5)
            {
                globalSettings->setValue("checkUpdateCounter", 0);
                checkForNewVersion();
            } else {
                globalSettings->setValue("checkUpdateCounter", globalSettings->value("checkUpdateCounter").toInt() + 1);
            }
        }
#endif

        miscMenu->setToolTipsVisible(true);
        miscMenu->addSeparator();

        auto legacyAction = miscMenu->addAction(QIcon(":/img/icone legacy.png"), tr("Enable legacy port"));
        //legacyAction->setCheckable(true);
        //legacyAction->setChecked(globalSettings->value("legacyport").toBool());
        if (legacyAction->isChecked())
        {
            legacyAction->setText(tr("Disable legacy port"));
        }
        legacyAction->setToolTip(tr("Enable the legacy port (8080), this is used by some old application like the MemoryViewer"));
        QObject::connect(legacyAction, &QAction::triggered, this, [=]() {
            if (globalSettings->value("legacyport").toBool() == false)
            {
                if (startLegacyPort())
                {
                    legacyAction->setText(tr("Disable legacy port"));
                    globalSettings->setValue("legacyport", true);
                }
            } else {
                globalSettings->setValue("legacyport", false);
                wsServer.stop(USB2SnesWS::legacyPort);
                legacyAction->setText(tr("Enable legacy port"));
            }
        });
        miscMenu->addAction(QIcon(":/img/line.svg"), tr("Diagnostic"));
        miscMenu->addSeparator();
        QObject::connect(miscMenu->addAction(QIcon(":/img/build.svg"), tr(" Diagnostic tool")), &QAction::triggered, this, [=] {
            DiagnosticDialog diag;
            diag.setWSServer(&wsServer);
            diag.exec();
        });
        QAction* debugLogAction = miscMenu->addAction(QIcon(":/img/analytic.svg"), tr("Enable debug logs"));
        debugLogAction->setCheckable(true);
        debugLogAction->setChecked(globalSettings->value("debugLog").toBool());
        if (debugLogAction->isChecked())
        {
            debugLogAction->setText("Disable debug log");
        }
        debugLogAction->setToolTip(tr("Enable the creation of a log file with lot of debug informations"));
        QObject::connect(debugLogAction, &QAction::changed, this, [=]() {
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
                    debugLogAction->setText("Disable debug log");
                }
                else {
                    debugLogAction->setChecked(false);
                }
            } else {
                sInfo() << "Debug log disabled";
                globalSettings->setValue("debugLog", false);
                debugLogAction->setText(tr("Enable debug logs"));
            }
        });
        auto openLogAction = miscMenu->addAction(QIcon(":/img/file.svg"), tr(" Open normal logs file"));
        QObject::connect(openLogAction, &QAction::triggered, this, [=] {
            QDesktopServices::openUrl(QUrl(SQPath::logDirectoryPath() + "/log.txt"));
        });
        auto openDebugLogAction = miscMenu->addAction(QIcon(":/img/file.svg"), tr(" Open debug logs file"));
        openDebugLogAction->setEnabled(globalSettings->value("debugLog").toBool());
        QObject::connect(openDebugLogAction, &QAction::triggered, this, [=] {
            QDesktopServices::openUrl(QUrl(SQPath::logDirectoryPath() + "/log-debug.txt"));
        });
        QObject::connect(menu->addAction(QIcon(":/img/quiticon.svg"), tr("Exit")), &QAction::triggered, qApp, &QApplication::exit);
        appsMenu->addSeparator();
        appsMenu->addAction(QIcon(":/img/line.svg"), tr("Remote Applications"));
        appsMenu->addSeparator();
        appsMenu->addAction(QIcon(":/img/multitroid.png"), "Multitroid");
        appsMenu->addAction(QIcon(":/img/BK small.png"), "SMZ3 Multiworld");
        appsMenu->addAction(QIcon(":/img/Funtoon.svg"), "Funtoon");

#ifdef  Q_OS_LINUX
    setLinuxDeviceMenu();
#endif
}

void AppUi::createApplicationMenu()
{
    if (regularApps.isEmpty())
        return ;
    appsMenu->clear();
    appsMenu->addAction(tr("Local Applications"));
    appsMenu->addSeparator();
    appsMenu->setToolTipsVisible(true);
    connect(appsMenu, SIGNAL(triggered(QAction*)), this, SLOT(onAppsMenuTriggered(QAction*)));
    for (const auto& key : regularApps.keys())
    {
        ApplicationInfo info = regularApps[key];
        sDebug() << "Adding " << info.name;
        sDebug() << info;
        QAction *act;
        QIcon icon;
        if (info.iconPath.isEmpty() == false)
        {
            if (info.desktopEntry == false)
            {
                icon = QIcon(info.iconPath);
            } else {
                icon = QIcon::fromTheme(info.iconPath);
            }
        } else  {
            if (ISWINDOWS())
            {
                QFileSystemModel model;
                QString exePath = QFileInfo(info.executablePath).absoluteFilePath();
                model.setRootPath(exePath);
                icon = model.fileIcon(model.index(exePath));
                act = appsMenu->addAction(model.fileIcon(model.index(exePath)), info.name);
            }
        }
        if (icon.isNull())
            act = appsMenu->addAction(info.name);
        else
            act = appsMenu->addAction(icon, info.name);
        act->setData(info.executablePath);
        if (!info.description.isEmpty())
            act->setToolTip(info.description);
    }
}

void AppUi::setLinuxDeviceMenu()
{
    linuxActionPos = 0;
    deviceMenu->addAction("Devices state");
    auto serverStatus = wsServer.serverStatus();
    for (quint8 i = 0; i < serverStatus.deviceFactoryCount * 3; i++)
    {
        deviceMenu->addAction(" ");
        linuxActionPos = i;
    }
    deviceMenu->addSeparator();
    onDeviceFactoryStatusDone();
}


void AppUi::onMenuHovered(QAction* action)
{
    //sDebug() << "Menu hovered";
    if (action != deviceMenu->menuAction() && action != nullptr)
        return ;
    if (checkingDeviceInfos)
        return ;
    //sDebug() << "Menu hovered code is running";
    checkingDeviceInfos = true;
    QTimer::singleShot(3000, this, [=] {
        checkingDeviceInfos = false;
    });
    auto serverStatus = wsServer.serverStatus();
    sDebug() << "Factory Count : " << serverStatus.deviceFactoryCount << "Devices Count: " << serverStatus.deviceCount;
    sDebug() << "Client count : " << serverStatus.clientCount;
#ifdef Q_OS_LINUX
    for (quint8 i = 0; i < linuxActionPos; i++)
    {
        deviceMenu->actions()[1 + i]->setText("");
    }
#endif
    linuxActionPos = 1;
#ifndef Q_OS_LINUX
    deviceMenu->clear();
    deviceMenu->addAction("Devices state");
    deviceMenu->addSeparator();
#endif
    UiWidget->initDeviceStatus();
    if (serverStatus.deviceFactoryCount == 0)
    {
        UiWidget->noDevicesStatus();
        deviceMenu->addAction(tr("No device activated, please enable one in the list bellow"));
    }
    UiWidget->clearClientStatus();
    for (const QString& clientName : wsServer.getAllClientsName())
    {
        UiWidget->addClientStatus(clientName);
    }
    sDebug() << "Asking the wserver to check status";
    wsServer.requestDeviceStatus();
}

void    AppUi::setDeviceEntry(const QString str)
{
#ifdef Q_OS_LINUX
    auto acts = deviceMenu->actions();
    acts[linuxActionPos]->setText(str);
    linuxActionPos++;
#else
    deviceMenu->addAction(str);
#endif
    UiWidget->addDeviceStatus(str);
}

void AppUi::onDeviceFactoryStatusReceived(DeviceFactory::DeviceFactoryStatus status)
{
    QString statusString;
    sDebug() << "Receveid status for" << status.name;
    if (status.deviceNames.isEmpty() == false)
    {
        setDeviceEntry(status.name + ":");
    } else {
        statusString = status.name + " : ";
    }
    //sDebug() << "Checking status";
    if (status.deviceNames.isEmpty())
    {
        //sDebug() << "No device, factory status";
        if (status.generalError == Error::DeviceFactoryError::DFE_NO_ERROR)
        {
            statusString.append(QString("%1").arg(status.statusString()));
        } else {
            statusString.append(QString("%1").arg(status.errorString()));
        }
    } else {
        //sDebug() << "Each device Status" << status.deviceNames;
        for(QString& name : status.deviceNames)
        {
            statusString = "       ";
            auto& deviceStatus = status.deviceStatus[name];
            if (deviceStatus.error == Error::DeviceError::DE_NO_ERROR)
            {
                QStringList clients = wsServer.getClientsName(name);
                //TODO move the name in the full translation string
                statusString.append(name + ": ");
                if (clients.isEmpty())
                {
                    statusString.append(tr("ready, no client connected"));
                } else {
                    statusString.append(clients.join(", "));
                }
            } else {
                statusString.append(name + " : " + deviceStatus.errorString());
            }
            sDebug() << "name" << statusString;
            setDeviceEntry(statusString);
            statusString.clear();
        }
    }
    if (status.deviceNames.isEmpty())
    {
        deviceMenu->addAction(statusString);
        UiWidget->addDeviceStatus(statusString);
        sDebug() << "Added devfact status : " << statusString;
    }
}

void AppUi::onDeviceFactoryStatusDone()
{
    deviceMenu->addSeparator();
    deviceMenu->addAction(sd2snesAction);
    deviceMenu->addAction(retroarchAction);
    //deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addAction(snesClassicAction);
    deviceMenu->addAction(emuNWAccessAction);
    //checkingDeviceInfos = false;
}


void AppUi::onAppsMenuTriggered(QAction *action)
{
    if (action->text() == "Multitroid")
        QDesktopServices::openUrl(QUrl("https://multitroid.com/"));
    if (action->text() == "SMZ3 Multiworld")
        QDesktopServices::openUrl(QUrl("https://samus.link"));
    if (action->text() == "Funtoon")
        QDesktopServices::openUrl(QUrl("https://funtoon.party/"));

    if (action->data().isNull())
        return ;

    ApplicationInfo appInfo = regularApps[action->data().toString()];
    QProcess proc(this);
    //proc.setWorkingDirectory(fi.path());
    QString exec;
    QFileInfo appFi(appInfo.executablePath);
    QString wDir = appFi.absolutePath();
    QStringList arg;
    if (appInfo.isQtApp)
    {
        wDir = qApp->applicationDirPath();
        if (ISWINDOWS())
        {
            arg << "-platformpluginpath" << qApp->applicationDirPath() + "/platforms/";
            if (QFileInfo::exists(wDir + "/styles/") == false)
            {
                QDir d(wDir);
                d.mkdir("styles");
                QFile::copy(qApp->applicationDirPath() + "/styles/qwindowsvistastyle.dll", wDir + "/styles/qwindowsvistastyle.dll");
            }
        }
    }
    exec = appInfo.executablePath;
    bool ok;
    if (appInfo.desktopEntry)
    {
        ok = proc.startDetached("/bin/sh", QStringList() << "-c" << appInfo.executablePath);
    } else {
        ok = proc.startDetached(exec, arg, wDir);
    }
    sInfo() << "Running " << exec << " in " << wDir << ok;
    if (!ok)
        sInfo() << "Error running " << exec << proc.errorString();
}
