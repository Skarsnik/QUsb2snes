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

#ifndef APPUI_H
#define APPUI_H

#include "../devices/luabridge.h"
#include "../devices/snesclassicfactory.h"
#include "../devices/sd2snesfactory.h"
#include "../devices/retroarchfactory.h"
#include "../devices/emunetworkaccessfactory.h"

#include <QMenu>
#include <QObject>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QLabel>
#include <QProgressBar>


class AppUi : public QObject
{
    Q_OBJECT
public:
    explicit    AppUi(QObject *parent = nullptr);
    void        init();
    void        updated(QString fromVersion);
    QSystemTrayIcon*    sysTray;

signals:

public slots:


private slots:
    void    onSD2SnesTriggered(bool checked);
    void    onRetroarchTriggered(bool checked);
    void    onLuaBridgeTriggered(bool checked);
    void    onSNESClassicTriggered(bool checked);
    void    onEmuNWAccessTriggered(bool checked);
    void    onAppsMenuTriggered(QAction* action);
    void    onMagic2SnesMenuTriggered(QAction*action);
    void    addWindowsSendToEntry();
    void    onUntrustedConnection(QString origin);
    void    DLManagerRequestFinished(QNetworkReply *reply);    
    void    onMenuHovered(QAction* action);
    void    onDeviceFactoryStatusReceived(DeviceFactory::DeviceFactoryStatus status);
    void    onDeviceFactoryStatusDone();

private:
    struct ApplicationInfo {
        bool    isQtApp;
        QString name;
        QString description;
        QString icon;
        QString folder;
        QString executable;
        ApplicationInfo() {
            isQtApp = false;
        }
        friend QDebug              operator<<(QDebug debug, const ApplicationInfo& req);
    };

    struct PopTrackerPackInfo
    {
        QString id;
        QVersionNumber version;
        QString description;
        friend QDebug              operator<<(QDebug debug, const PopTrackerPackInfo& req);
    };
    friend QDebug              operator<<(QDebug debug, const AppUi::PopTrackerPackInfo& req);
    friend QDebug              operator<<(QDebug debug, const AppUi::ApplicationInfo& req);
    QMenu*              menu;
    QMenu*              deviceMenu;
    QMenu*              appsMenu;
    QMenu*              magic2SnesMenu;
    QMenu*              popTrackerMenu;
    QMenu*              miscMenu;
    QString             magic2SnesExe;
    QAction*            sd2snesAction;
    QAction*            retroarchAction;
    QAction*            snesClassicAction;
    QAction*            luaBridgeAction;
    QAction*            emuNWAccessAction;

    LuaBridge*                      luaBridge;
    SD2SnesFactory*                 sd2snesFactory;
    RetroArchFactory*               retroarchFactory;
    SNESClassicFactory*             snesClassic;
    EmuNetworkAccessFactory*        emuNWAccess;
    QMap<QString, ApplicationInfo>  regularApps;

    QNetworkAccessManager*          dlManager;
    QLabel*                         dlLabel;
    QProgressBar*                   dlProgressBar;
    QWidget*                        dlWindow;
    bool                            checkingDeviceInfos;
    QString                         popTrackerExePath;

    quint8                          linuxActionPos;

    void                checkForApplications();
    void                handleMagic2Snes(QString path);

    void                addMagic2SnesFolder(QString path);
    void                addDevicesInfo(DeviceFactory *devFact);
    ApplicationInfo     parseJsonAppInfo(QString fileName);
    void                checkForNewVersion(bool manual = false);
    void startWServer();
    void addSD2SnesFactory();
    void addRetroarchFactory();
    void addLuaBridgeFactory();
    void addSnesClassicFactory();
    void addNWAFactory();
    void setLinuxDeviceMenu();
    void setMenu();
    void setDeviceEntry(const QString str);
    QList<PopTrackerPackInfo> poptrackerScanPack();
    bool checkPopTracker();
    void addPopTrackerMenu();
};

#endif // APPUI_H
