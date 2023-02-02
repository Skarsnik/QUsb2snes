#include <QLoggingCategory>
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include "appui.h"

#include "diagnosticdialog.h"
#include "../wsserver.h"

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
    }
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
        QAction* debugLogAction = miscMenu->addAction(tr("Enable debug logs"));
        debugLogAction->setCheckable(true);
        debugLogAction->setChecked(globalSettings->value("debugLog").toBool());
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
                }
                else {
                    debugLogAction->setChecked(false);
                }
            } else {
                sInfo() << "Debug log disabled";
                globalSettings->setValue("debugLog", false);
            }
        });
        QObject::connect(miscMenu->addAction(tr("Diagnostic tool")), &QAction::triggered, this, [=] {
           DiagnosticDialog diag;
           diag.setWSServer(&wsServer);
           diag.exec();
        });
        QObject::connect(menu->addAction(tr("Exit")), &QAction::triggered, qApp, &QApplication::exit);
        appsMenu->addSeparator();
        appsMenu->addAction(tr("Remote Applications"));
        appsMenu->addSeparator();
        appsMenu->addAction(QIcon(":/img/multitroid.png"), "Multitroid");
        appsMenu->addAction(QIcon(":/img/BK small.png"), "SMZ3 Multiworld");

#ifdef  Q_OS_LINUX
    setLinuxDeviceMenu();
#endif
}

void AppUi::setLinuxDeviceMenu()
{
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
    if (action != deviceMenu->menuAction())
        return ;
    if (checkingDeviceInfos)
        return ;
    checkingDeviceInfos = true;
    QTimer::singleShot(4000, this, [=] {
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
}

void AppUi::onDeviceFactoryStatusReceived(DeviceFactory::DeviceFactoryStatus status)
{
    QString statusString;
    sDebug() << "Receveid status for" << status.name;
    if (!status.deviceNames.isEmpty())
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
        sDebug() << "Added devfact status : " << statusString;
    }
}

void AppUi::onDeviceFactoryStatusDone()
{
    deviceMenu->addSeparator();
    deviceMenu->addAction(sd2snesAction);
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addAction(luaBridgeAction);
    deviceMenu->addAction(snesClassicAction);
    deviceMenu->addAction(emuNWAccessAction);
    //checkingDeviceInfos = false;
}


void AppUi::onAppsMenuTriggered(QAction *action)
{
    if (action->text() == "Multitroid")
        QDesktopServices::openUrl(QUrl("http://multitroid.com/"));
    if (action->text() == "SMZ3 Multiworld")
        QDesktopServices::openUrl(QUrl("https://samus.link"));
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
        if (!QFileInfo::exists(appInfo.folder + "/styles/"))
        {
            QDir d(appInfo.folder);
            d.mkdir("styles");
            QFile::copy(qApp->applicationDirPath() + "/styles/qwindowsvistastyle.dll", appInfo.folder + "/styles/qwindowsvistastyle.dll");
        }
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
