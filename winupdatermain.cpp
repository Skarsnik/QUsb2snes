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


const QString githubUrl = "https://api.github.com/repos/Skarsnik/QUsb2snes/";
int step = 0;
QNetworkAccessManager* manager = new QNetworkAccessManager();
QNetworkReply* dlReply;
QProgressBar* pb;
QLabel* label;

void    requestFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    qDebug() << reply->error();
    if (step == 0)
    {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray jArr = doc.array();
        QJsonArray assets = jArr.at(0).toObject().value("assets").toArray();
        foreach(const QJsonValue& value, assets)
        {
            if (value.toObject().value("name").toString() == "QUsb2Snes.exe")
            {
                qDebug() << "Downloading " << value.toObject().value("browser_download_url").toString();
                label->setText(QString(QObject::tr("Downloading new QUsb2Snes.exe %1")).arg(
                                  jArr.at(0).toObject().value("tag_name").toString()));
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                dlReply = manager->get(req);
                QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
                   qDebug() << "DL reply redirected";
                });
                QObject::connect(dlReply, &QNetworkReply::downloadProgress, [=](qint64 bytesRcv, qint64 bytesTotal)
                {
                    qDebug() << 20 + (bytesRcv / bytesTotal) * 80;
                    pb->setValue(20 + (bytesRcv / bytesTotal) * 80);
                });
                qDebug() << "Found QUsb2Snes.exe asset";
                step = 1;
                return ;
            }
        }
        qApp->exit(1);
     }
     if (step == 1)
     {
         QFile file("QUsb2Snes.exe");

         pb->setValue(100);
         label->setText(QObject::tr("Writing QUsb2Snes.exe"));
         if (file.open(QIODevice::WriteOnly))
         {
            qDebug() << data.size();
            file.write(data);
            qDebug() << "QUsb2Snes written";
            file.flush();
            file.close();
            label->setText(QObject::tr("Starting QUsb2Snes"));
            QProcess::startDetached("QUsb2Snes.exe");
            QTimer::singleShot(500, [=] {qApp->exit(0);});
         } else {
            qApp->exit(1);
         }
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
    QTimer::singleShot(1000, &startDelayed);

    label = new QLabel();
    pb = new QProgressBar();
    label->setText(QObject::tr("Checking GitHub for the last release"));
    pb->setTextVisible(false);
    pb->setValue(20);
    QVBoxLayout layout;
    QWidget win;
    win.setWindowTitle("QUsb2Snes Windows Updater");
    win.setLayout(&layout);
    layout.addWidget(label);
    layout.addWidget(pb);
    win.show();
    app.exec();
}
