#include <QApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_appUi, "APPUI")
#define sDebug() qCDebug(log_appUi)

#include "appui.h"
#include "wsserver.h"



extern WSServer    wsServer;

AppUi::AppUi(QObject *parent) : QObject(parent)
{
    sysTray = new QSystemTrayIcon(QIcon(":/img/icon64x64.ico"));
    menu = new QMenu();
    menu->addAction("QUsb2Snes v" + QApplication::applicationVersion());
    menu->addSeparator();
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToshow()));
    deviceMenu = menu->addMenu("Devices");
    retroarchAction = new QAction("Enable RetroArch virtual device");
    retroarchAction->setCheckable(true);
    connect(retroarchAction, SIGNAL(triggered(bool)), this, SLOT(onRetroarchTriggered(bool)));
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addSeparator();
    QObject::connect(menu->addAction("Exit"), &QAction::triggered, qApp, &QApplication::exit);
    sysTray->setContextMenu(menu);
    retroarchDevice = NULL;
    settings = new QSettings("config.ini", QSettings::IniFormat);
    if (settings->value("retroarchdevice").toBool())
    {
        retroarchDevice = new RetroarchDevice();
        wsServer.addDevice(retroarchDevice);
        retroarchAction->setChecked(true);
    }
}

void AppUi::onRetroarchTriggered(bool checked)
{
    if (checked == true)
    {
        if (retroarchDevice == NULL)
            retroarchDevice = new RetroarchDevice();
        //retroarchAction->setText("Enable RetroArch virtual device");
        wsServer.addDevice(retroarchDevice);
    } else {
        wsServer.removeDevice(retroarchDevice);
    }
    settings->setValue("retroarchdevice", checked);
}

void AppUi::onMenuAboutToshow()
{
    deviceMenu->clear();
    deviceMenu->addAction(retroarchAction);
    deviceMenu->addSection("Devices state");
    auto piko = wsServer.getDevicesInfo();
    QMapIterator<QString, QStringList> it(piko);
    while (it.hasNext())
    {
        auto p = it.next();
        deviceMenu->addAction(p.key() + " : " + p.value().join(" - "));
    }
}
