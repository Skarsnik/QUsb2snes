#ifndef APPUI_H
#define APPUI_H

#include "retroarchdevice.h"

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
    void    onMenuAboutToshow();

private:
    QMenu*              menu;
    QMenu*              deviceMenu;
    QAction*            retroarchAction;
    RetroarchDevice*    retroarchDevice;
    QSettings*           settings;

};

#endif // APPUI_H
