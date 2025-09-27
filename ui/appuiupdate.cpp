#include <QLoggingCategory>
#include <QApplication>
#include <QJsonObject>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QVBoxLayout>
#include <QProcess>
#include "appui.h"



Q_LOGGING_CATEGORY(log_appUiUpdate, "APPUiUpdate")
#define sDebug() qCDebug(log_appUiUpdate)
#define sInfo() qCInfo(log_appUiUpdate)

const QString               telemetryUrl = "https://telemetry.nyo.fr/api/rpc/add_qusb2snes_usage";
const QString               telemetryTestUrl = "http://192.168.186.1/api/rpc/add_qusb2snes_usage";
extern QSettings*           globalSettings;

static QNetworkReply*   telemetryReply = nullptr;

void AppUi::initDLManager()
{
    dlManager = new QNetworkAccessManager();
    dlManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QObject::connect(dlManager, &QNetworkAccessManager::finished, this, &AppUi::DLManagerRequestFinished);
}

void AppUi::checkForNewVersion(bool manual)
{
    sInfo() << "Checking for new version - SSL support is : " << QSslSocket::supportsSsl() << QSslSocket::sslLibraryBuildVersionString();
    if (!QSslSocket::supportsSsl())
        return;
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply)
                     {
                         sDebug() << "Finished" << reply->size();
                         QByteArray data = reply->readAll();
                         QJsonDocument doc = QJsonDocument::fromJson(data);
                         QJsonArray jArr = doc.array();
                         QString lastTag = jArr.at(0).toObject().value("tag_name").toString();
                         QString body = jArr.at(0).toObject().value("body").toString();
                         QString name = jArr.at(0).toObject().value("name").toString();
                         sInfo() << "Latest release is " << lastTag;
                         sDebug() << body;
                         body.replace('\n', "<br/>");
                         body.replace('\r', "");

                         if (QVersionNumber::fromString(qApp->applicationVersion()) < QVersionNumber::fromString(lastTag.remove(0, 1)))
                         {
                             QMessageBox msg;
                             msg.setText(QString(tr("A new version of QUsb2Snes is available : QUsb2Snes %1\nDo you want to upgrade to it?")).arg(lastTag));
                             msg.setWindowTitle(tr("New version of QUsb2Snes available"));
                             msg.setInformativeText(QString("<b>%1</b><p>%2</p>").arg(name).arg(body));
                             msg.addButton(QMessageBox::Yes);
                             msg.addButton(QMessageBox::No);
                             msg.setDefaultButton(QMessageBox::No);
                             int but = msg.exec();
                             if (but == QMessageBox::Yes)
                             {
                                 if (dlManager == nullptr)
                                 {
                                     initDLManager();
                                     dlLabel = new QLabel();
                                     dlProgressBar = new QProgressBar();
                                     dlProgressBar->setTextVisible(false);
                                     dlLabel->setText(tr("Downloading the Windows Updater"));
                                     dlWindow = new QWidget();
                                     QVBoxLayout* layout = new QVBoxLayout();
                                     dlWindow->setWindowTitle("Updating Updater");
                                     dlWindow->setWindowIcon(QIcon(":/icon64x64.ico"));
                                     dlWindow->setLayout(layout);
                                     layout->addWidget(dlLabel);
                                     layout->addWidget(dlProgressBar);
                                 }
                                 dlWindow->show();
                                 dlManager->get(QNetworkRequest(QUrl("https://api.github.com/repos/Skarsnik/QUsb2Snes/releases")));
                                 //QString upExe(qApp->applicationDirPath() + "/WinUpdater.exe");
                                 //(int)::ShellExecute(0, reinterpret_cast<const WCHAR*>("runas"), reinterpret_cast<const WCHAR*>(upExe.utf16()), 0, 0, SW_SHOWNORMAL);
                                 //QProcess::startDetached(upExe);
                                 //qApp->exit(0);
                             }
                         } else {
                             if (manual)
                                 QMessageBox::information(nullptr, tr("No new version of QUsb2Snes available"), tr("No new version of QUsb2Snes available"));
                         }
                     });
    manager->get(QNetworkRequest(QUrl("https://api.github.com/repos/Skarsnik/QUsb2snes/releases")));
    sDebug() << "Get";
}

// Code to download the updater

void    AppUi::DLManagerRequestFinished(QNetworkReply* reply)
{
    static int step = 0;

    if (telemetryReply != nullptr && reply == telemetryReply)
    {
        sDebug() << "Telemetry status code : " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        return ;
    }
    QByteArray data = reply->readAll();
    qDebug() << reply->error();
    if (step == 0)
    {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jArr = doc.array();
        QJsonArray assets = jArr.at(0).toObject().value("assets").toArray();
        foreach(const QJsonValue& value, assets)
        {
            if (value.toObject().value("name").toString() == "WinUpdater.exe")
            {
                sDebug() << "Downloading " << value.toObject().value("browser_download_url").toString();
                dlLabel->setText(QString(QObject::tr("Downloading new WinUpdater %1")).arg(
                    jArr.at(0).toObject().value("tag_name").toString()));
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                QNetworkReply*  dlReply = dlManager->get(req);
                QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
                    sDebug() << "DL reply redirected";
                });
                QObject::connect(dlReply, &QNetworkReply::downloadProgress, this, [=](qint64 bytesRcv, qint64 bytesTotal)
                                 {
                                     qDebug() << 20 + (bytesRcv / bytesTotal) * 80;
                                     dlProgressBar->setValue(20 + (bytesRcv / bytesTotal) * 80);
                                 });
                sDebug() << "Found WinUpdater.exe asset";
                step = 1;
                return ;
            }
        }
        qApp->exit(1);
    }
    if (step == 1)
    {
        QFile file(qApp->applicationDirPath() + "/WinUpdater.exe");

        step = 0;
        dlProgressBar->setValue(100);
        dlLabel->setText(QObject::tr("Writing WinUpdater.exe"));
        if (file.open(QIODevice::WriteOnly))
        {
            sDebug() << data.size();
            file.write(data);
            sDebug() << "WinUpdater written";
            file.flush();
            file.close();
            dlLabel->setText(QObject::tr("Starting the Windows Updater"));
            QProcess::startDetached(qApp->applicationDirPath() + "/WinUpdater.exe");
            QTimer::singleShot(200, this, [=] {qApp->exit(0);});
        } else {
            dlWindow->hide();
            QMessageBox::warning(nullptr, tr("Error downloading the updater"), tr("There was an error downloading or writing the updater file, please try to download it manually on the release page"));
        }
    }
}

void AppUi::updated(QString fromVersion)
{
    QMessageBox::about(nullptr, tr("QUsb2Snes updated succesfully"),
                       QString(tr("QUsb2Snes successfully updated to version %2")).arg(fromVersion).arg(qApp->applicationVersion()));
    if (false == globalSettings->contains("telemetrydatahandled"))
    {
        QStringList devices;

        if (globalSettings->value("sd2snessupport").toBool())
            devices << "SD2SNES";
        if (globalSettings->value("retroarchdevice").toBool())
            devices << "RETROARCH";
        if (globalSettings->value("luabridge").toBool())
            devices << "LUABRIDGE";
        if (globalSettings->value("snesclassic").toBool())
            devices << "SNESCLASSIC";
        if (globalSettings->value("emunwaccess").toBool())
            devices << "NWA";
        auto ok = QMessageBox::question(nullptr, tr("QUsb2Snes telemetry"), QString(tr("QUsb2Snes can collect informations to have a better "
                                 "understanding of its users. \nWe don't collect personnal data, here what will be collected :\n"
                                                                            "QUsb2Snes' version : %1 - Device(s) selected : [%3] - System : %2\n\nDo you agree to send this?")
                                ).arg(qApp->applicationVersion(), QSysInfo::prettyProductName(), devices.join(", ")));
        if (ok == QMessageBox::Yes)
        {
            postTelemetryData();
        }
        globalSettings->setValue("telemetrydatahandled", true);
    }
}


void AppUi::postTelemetryData()
{
    sInfo() << "Sending telemetry datas";
    QString os = QSysInfo::productType();
    QString fullOsVersion = QSysInfo::prettyProductName();
    QString version = qApp->applicationVersion();
    QStringList devices;

    if (globalSettings->value("sd2snessupport").toBool())
        devices << "SD2SNES";
    if (globalSettings->value("retroarchdevice").toBool())
        devices << "RETROARCH";
    if (globalSettings->value("luabridge").toBool())
        devices << "LUABRIDGE";
    if (globalSettings->value("snesclassic").toBool())
        devices << "SNESCLASSIC";
    if (globalSettings->value("emunwaccess").toBool())
        devices << "NWA";
    QJsonObject jsonObj;
    jsonObj["os"] = os;
    jsonObj["devices"] = QJsonArray::fromStringList(devices);
    jsonObj["version"] = version;
    jsonObj["os_details"] = fullOsVersion;
    QByteArray toSend = QJsonDocument(jsonObj).toJson();
    QNetworkRequest req;
    req.setUrl(QUrl(telemetryUrl));
    //req.setUrl(QUrl(telemetryTestUrl));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    sDebug() << req.rawHeaderList();
    sInfo() << toSend;
    if (dlManager == nullptr)
        initDLManager();
    telemetryReply = dlManager->post(req, toSend);
}
