#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>

const QString githubUrl = "https://api.github.com/repos/Skarsnik/QUsb2snes/";
int step = 0;
QNetworkAccessManager* manager = new QNetworkAccessManager();
QNetworkReply* dlReply;


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
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                dlReply = manager->get(req);
                QObject::connect(dlReply, &QNetworkReply::redirected, [=] {
                   qDebug() << "DL reply redirected";
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

         if (file.open(QIODevice::WriteOnly))
         {
            qDebug() << data.size();
            file.write(data);
            qDebug() << "QUsb2Snes written";
            file.flush();
            file.close();
            QProcess::startDetached("QUsb2Snes.exe");
            qApp->exit(0);
         } else {
            qApp->exit(1);
         }
     }
}

int main(int ac, char* ag[])
{
    QCoreApplication app(ac, ag);
    QObject::connect(manager, &QNetworkAccessManager::finished, &requestFinished);
    manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    manager->get(QNetworkRequest(QUrl(githubUrl + "releases")));
    app.exec();
}
