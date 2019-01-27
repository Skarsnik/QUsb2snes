#ifndef APPUI_H
#define APPUI_H

#include "devices/luabridge.h"
#include "devices/luabridgedevice.h"
#include "devices/retroarchdevice.h"
#include "devices/snesclassicfactory.h"
#include "devices/snesclassic.h"
#include "devices/sd2snesfactory.h"

#include <QMenu>
#include <QObject>
#include <QSettings>
#include <QSystemTrayIcon>

class AppUi : public QObject
{
    Q_OBJECT
public:
    explicit AppUi(QObject *parent = nullptr);
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
    QMenu*              menu;
    QMenu*              deviceMenu;
    QMenu*              appsMenu;
    QMenu*              magic2SnesMenu;
    QMenu*              miscMenu;
    QString             magic2SnesExe;
    QAction*            retroarchAction;
    QAction*            snesClassicAction;
    QAction*            luaBridgeAction;
    LuaBridge*          luaBridge;
    SD2SnesFactory*     sd2snesFactory;
    RetroarchDevice*    retroarchDevice;
    SNESClassicFactory* snesClassic;
    QSettings*          settings;
    QMap<QString, QString>  regularApps;

    void                checkForApplications();
    void                handleMagic2Snes(QString path);

    void                addMagic2SnesFolder(QString path);
    void addDevicesInfo(DeviceFactory *devFact);
};

#endif // APPUI_H
