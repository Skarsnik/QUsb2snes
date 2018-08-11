#ifndef APPUI_H
#define APPUI_H

#include "luabridge.h"
#include "retroarchdevice.h"
#include "snesclassic.h"

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

private:
    QMenu*              menu;
    QMenu*              deviceMenu;
    QMenu*              appsMenu;
    QMenu*              magic2SnesMenu;
    QString             magic2SnesExe;
    QAction*            retroarchAction;
    QAction*            snesClassicAction;
    QAction*            luaBridgeAction;
    LuaBridge*          luaBridgeDevice;
    RetroarchDevice*    retroarchDevice;
    SNESClassic*        snesClassicDevice;
    QSettings*          settings;
    QMap<QString, QString>  regularApps;

    void                checkForApplications();
    void                handleMagic2Snes(QString path);

    void addMagic2SnesFolder(QString path);
};

#endif // APPUI_H
