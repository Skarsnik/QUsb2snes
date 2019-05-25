#ifndef APPUI_H
#define APPUI_H

#include "devices/luabridge.h"
#include "devices/snesclassicfactory.h"
#include "devices/sd2snesfactory.h"
#include "devices/retroarchfactory.h"

#include <QMenu>
#include <QObject>
#include <QSettings>
#include <QSystemTrayIcon>


class AppUi : public QObject
{
    Q_OBJECT
public:
    explicit AppUi(QObject *parent = nullptr);
    void     init();
    QSystemTrayIcon*    sysTray;

signals:

public slots:


private slots:
    void    onRetroarchTriggered(bool checked);
    void    onLuaBridgeTriggered(bool checked);
    void    onSNESClassicTriggered(bool checked);
    void    onMenuAboutToshow();
    void    onAppsMenuTriggered(QAction* action);
    void    onMagic2SnesMenuTriggered(QAction*action);
    void    addWindowsSendToEntry();
    void    onUntrustedConnection(QString origin);

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

    friend QDebug              operator<<(QDebug debug, const AppUi::ApplicationInfo& req);
    QMenu*              menu;
    QMenu*              deviceMenu;
    QMenu*              appsMenu;
    QMenu*              magic2SnesMenu;
    QMenu*              miscMenu;
    QString             magic2SnesExe;
    QAction*            retroarchAction;
    QAction*            snesClassicAction;
    QAction*            luaBridgeAction;
    LuaBridge*                      luaBridge;
    SD2SnesFactory*                 sd2snesFactory;
    RetroArchFactory*               retroarchFactory;
    SNESClassicFactory*             snesClassic;
    QMap<QString, ApplicationInfo>  regularApps;

    void                checkForApplications();
    void                handleMagic2Snes(QString path);

    void                addMagic2SnesFolder(QString path);
    void                addDevicesInfo(DeviceFactory *devFact);
    ApplicationInfo     parseJsonAppInfo(QString fileName);
    void                checkForNewVersion(bool manual = false);
};

#endif // APPUI_H
