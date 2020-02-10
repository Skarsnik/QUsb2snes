#include <QApplication>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QIcon>
#include <QMessageBox>


const QString githubUrl = "https://api.github.com/repos/Skarsnik/QUsb2snes/";
int step = 0;
QNetworkAccessManager* manager = new QNetworkAccessManager();
QNetworkReply* dlReply;
QProgressBar* pb;
QLabel* label;
QList<QNetworkRequest> listReq;
QStringList exeToDL = {"QUsb2Snes.exe", "QFile2Snes.exe"};
QString newQUsbVersion;



void    startQUsb2Snes()
{
    qDebug() << "Starting QUsb2Snes";
    label->setText(QObject::tr("Starting QUsb2Snes"));
    QProcess::startDetached(qApp->applicationDirPath() + "/QUsb2Snes.exe", QStringList() << "-updated" << newQUsbVersion);
    qDebug() << "starting exit";
    QTimer::singleShot(1000, [=] {qApp->exit(0);});
}


void    requestFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    qDebug() << reply->error();
    if (step == 0)
    {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jArr = doc.array();
        QJsonArray assets = jArr.at(0).toObject().value("assets").toArray();
        newQUsbVersion = jArr.at(0).toObject().value("tag_name").toString();
        foreach(const QJsonValue& value, assets)
        {
            if (value.toObject().value("name").toString() == "QUsb2Snes.exe")
            {
                qDebug() << "Downloading " << value.toObject().value("browser_download_url").toString();
                label->setText(QString(QObject::tr("Downloading new QUsb2Snes.exe %1")).arg(
                                  jArr.at(0).toObject().value("tag_name").toString()));
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                listReq.append(req);
                qDebug() << "Found QUsb2Snes.exe asset";
            }
            if (value.toObject().value("name").toString() == "QFile2Snes.exe")
            {
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                listReq.append(req);
            }
        }
        step = 1;
        dlReply = manager->get(listReq.takeFirst());
        QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
           qDebug() << "DL reply redirected";
        });
        QObject::connect(dlReply, &QNetworkReply::downloadProgress, [=](qint64 bytesRcv, qint64 bytesTotal)
        {
            qDebug() << 20 + (bytesRcv / bytesTotal) * 80;
            pb->setValue(20 + (bytesRcv / bytesTotal) * 80);
        });

        return ;
     }
     if (step >= 1)
     {
         QString fileStr = exeToDL.takeFirst();
         QFile file(fileStr);

         pb->setValue(100);
         label->setText(QString(QObject::tr("Writing %1")).arg(fileStr));
         if (file.open(QIODevice::WriteOnly))
         {
            qDebug() << data.size();
            file.write(data);
            qDebug() << fileStr << " written";
            file.flush();
            file.close();
         } else {
            QMessageBox::critical(nullptr, QString(QObject::tr("Error writing %1")).arg(fileStr),
                                  QObject::tr("There was an error writing the new QUsb2Snes, check if QUsb2Snes is not running and restart the update (or the updater)"));
            qApp->exit(1);
         }
         if (listReq.isEmpty()) {
             startQUsb2Snes();
             return ;
         }
         pb->setValue(0);
         dlReply = manager->get(listReq.takeFirst());
         label->setText(QString(QObject::tr("Downloading new %1")).arg(exeToDL.first()));
         QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
            qDebug() << "DL reply redirected";
         });
         QObject::connect(dlReply, &QNetworkReply::downloadProgress, [=](qint64 bytesRcv, qint64 bytesTotal)
         {
             qDebug() << 20 + (bytesRcv / bytesTotal) * 80;
             pb->setValue(20 + (bytesRcv / bytesTotal) * 80);
         });
     }
}



void    startDelayed()
{
    manager->get(QNetworkRequest(QUrl(githubUrl + "releases")));
}

int main(int ac, char* ag[])
{
    QApplication app(ac, ag);
    QObject::connect(manager, &QNetworkAccessManager::finished, &requestFinished);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QTimer::singleShot(1500, &startDelayed);

    label = new QLabel();
    pb = new QProgressBar();
    label->setText(QObject::tr("Checking GitHub for the last release"));
    pb->setTextVisible(false);
    pb->setValue(20);
    QVBoxLayout layout;
    QWidget win;
    win.setWindowTitle("QUsb2Snes Windows Updater");
    win.setWindowIcon(QIcon(":/icon64x64.ico"));
    win.setLayout(&layout);
    layout.addWidget(label);
    layout.addWidget(pb);
    win.show();
    app.exec();
}
