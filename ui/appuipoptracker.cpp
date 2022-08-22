#include <QApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_appPopTracker, "APPUIPopTracker")
#define sDebug() qCDebug(log_appPopTracker)
#define sInfo() qCInfo(log_appPopTracker)

#include <QProcess>
#include "appui.h"




bool AppUi::checkPopTracker()
{
    popTrackerExePath = qApp->applicationDirPath() + "/poptracker/poptracker.exe";
    return QFile::exists(qApp->applicationDirPath() + "/poptracker/poptracker.exe");
}

void    AppUi::addPopTrackerMenu()
{
    sDebug() << "Creating Poptracker menu";
    popTrackerMenu = menu->addMenu(QIcon(":/img/poptrackericon.png"), "PopTracker");
    auto packList = poptrackerScanPack();
    if (packList.isEmpty())
    {
        popTrackerMenu->addAction(tr("PopTracker detected but no pack installed"));
        popTrackerMenu->addSeparator();
        connect(popTrackerMenu->addAction(QIcon(":/img/poptrackericon.png"), tr("Start PopTracker")), &QAction::triggered, this, [path = popTrackerExePath](bool trigger)
        {
            QFileInfo fi(path);
            QProcess::startDetached(path, QStringList(), fi.absolutePath());
        });
        return ;
    }
    popTrackerMenu->addAction(tr("Installed packs", "Poptracker pack"));
    popTrackerMenu->addSeparator();
    for (const auto& packInfo : qAsConst(packList))
    {
        QAction* act = popTrackerMenu->addAction(packInfo.description);
        act->setToolTip(packInfo.id + " " + packInfo.version.toString());
        connect(act, &QAction::triggered, this, [path = popTrackerExePath, id = packInfo.id](bool trigger)
        {
            QProcess::startDetached(path, QStringList() << "--load-pack" << id);
        }
        );
    }
}

QList<AppUi::PopTrackerPackInfo> AppUi::poptrackerScanPack()
{
    QList<PopTrackerPackInfo> toret;
    QProcess poptrackerProcess(this);
    poptrackerProcess.start(qApp->applicationDirPath() + "/poptracker/poptracker.exe", QStringList() << "--list-installed");
    if (!poptrackerProcess.waitForFinished(2000))
        return toret;

    QByteArray data = poptrackerProcess.readAllStandardOutput();
    QTextStream stream(data);
    QString line = stream.readLine();
    while (!stream.atEnd())
    {
        line = stream.readLine();
        if (line.indexOf("no packs installed") != -1 || line.isEmpty())
            break;
        sDebug() << line;
        auto pl = line.split(" ");
        PopTrackerPackInfo pi;
        pi.id = pl.at(0);
        pi.version = QVersionNumber::fromString(pl.at(1));
        pi.description = pl.mid(2).join(" ");
        pi.description = pi.description.mid(1, pi.description.size() - 2);
        sInfo() << "Adding pack : " << pi;
        toret << pi;
    }
    return toret;
}


QDebug              operator<<(QDebug debug, const AppUi::PopTrackerPackInfo& req)
{
    debug << req.id << req.version << req.description;
    return debug;
}
