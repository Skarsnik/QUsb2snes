/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

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
QStringList         listReqExe;
QStringList exeToDL = {"QUsb2Snes.exe", "QFile2Snes.exe"};
QString newQUsbVersion;
QMap<QString, QString> locations;

static QTextStream logfile;
static QTextStream cout(stdout);

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QTextStream*    log = &logfile;
#ifdef QT_NO_DEBUG
    QString logString = QString("%6 %5 - %7: %1").arg(localMsg.constData()).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
#else
    QString logString = QString("%6 %5 - %7: %1 \t(%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
#endif
    switch (type)
    {
        case QtDebugMsg:
            *log << logString.arg("Debug");
            break;
        case QtCriticalMsg:
            *log << logString.arg("Critical");
            break;
        case QtWarningMsg:
            *log << logString.arg("Warning");
            break;
        case QtFatalMsg:
            *log << logString.arg("Fatal");
            *log<< "\n"; log->flush();
            QMessageBox::critical(nullptr, QObject::tr("Critical error"), msg);
            qApp->exit(1);
            break;
        case QtInfoMsg:
            *log << logString.arg("Info");
            break;
    }
    *log << "\n";
    log->flush();
    cout << QString("%1 : %2").arg(context.category, 20).arg(msg) << "\n";
    cout.flush();
}


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
                listReqExe.append("QUsb2Snes.exe");
                qDebug() << "Found QUsb2Snes.exe asset";
            }
            if (value.toObject().value("name").toString() == "QFile2Snes.exe")
            {
                QNetworkRequest req(QUrl(value.toObject().value("browser_download_url").toString()));
                req.setRawHeader("Accept",  "application/octet-stream");
                qDebug() << "Found QFile2Snes.exe asset";
                listReq.append(req);
                listReqExe.append("QFile2Snes.exe");
            }
        }
        step = 1;
        label->setText(QString(QObject::tr("Downloading new %1")).arg(listReqExe.first()));
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
         QString fileStr = listReqExe.takeFirst();
         QFile file(locations[fileStr] + "/" + fileStr);
         qDebug() << "Creating " << file.fileName();
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
            qDebug() << "Can't write file";
            QMessageBox::critical(nullptr, QString(QObject::tr("Error writing %1")).arg(fileStr),
                                  QString(QObject::tr("There was an error writing %1, check if %1 is not running and restart the update (or the updater)")).arg(fileStr));
            qApp->exit(1);
         }
         if (listReq.isEmpty()) {
             startQUsb2Snes();
             return ;
         }
         pb->setValue(0);
         dlReply = manager->get(listReq.takeFirst());
         label->setText(QString(QObject::tr("Downloading new %1")).arg(listReqExe.first()));
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
    locations["QUsb2Snes.exe"] = qApp->applicationDirPath();
    locations["QFile2Snes.exe"] = qApp->applicationDirPath() + "/apps/QFile2Snes/";
    QFile   mlog(qApp->applicationDirPath() + "/updaterlog.txt");
    if (mlog.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        logfile.setDevice(&mlog);
        qInstallMessageHandler(myMessageOutput);
    }

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
