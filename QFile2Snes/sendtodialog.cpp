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

#include "dirchangedialog.h"
#include "sendtodialog.h"
#include "ui_sendtodialog.h"
#include <QDebug>
#include <QMessageBox>

static const QString UPDIRKEY = "UploadDir";
static const int SECONDBEFORESTART = 3;
static const QString EXECAFTERUP = "ExecAfterUpload";


SendToDialog::SendToDialog(QString filePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendToDialog)
{
    ui->setupUi(this);
    fileInfos.setFile(filePath);
    ui->filenameLabel->setText(fileInfos.fileName());
    qDebug() << fileInfos.filePath();
    if (fileInfos.suffix() == "sfc" || fileInfos.suffix() == "smc")
        ui->romStartCheckBox->setEnabled(true);
    usb2snes = nullptr;
    setWindowTitle(QString(tr("Send %1 to SD2SNES").arg(fileInfos.fileName())));

    settings = new QSettings("skarsnik.nyo.fr", "QFile2Snes");
    if (!settings->contains(UPDIRKEY))
    {
        settings->setValue(UPDIRKEY, "/");
    }
    ui->dirLineEdit->setText(settings->value(UPDIRKEY).toString());
    if (settings->contains(EXECAFTERUP))
        ui->romStartCheckBox->setChecked(settings->value(EXECAFTERUP).toBool());

    connect(&progressTimer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
    connect(&recoTimer, SIGNAL(timeout()), this, SLOT(onRecoTimerTimeout()));
    countdownString = tr("Connected to the server - Starting transfert in %1 seconds");
    sendingFile = false;
    progressStep = 0;
}

bool SendToDialog::init()
{
    if (!checkForUsb2SnesServer())
    {
        QMessageBox::critical(this, tr("QFile2Snes error"), tr("Cannot connect to a usb2snes application or can't start one"));
        return false;
    }
    usb2snes = new Usb2Snes(false);
    connect(usb2snes, SIGNAL(stateChanged()), this, SLOT(onUsb2SnesStateChanged()));
    connect(usb2snes, SIGNAL(disconnect()), this, SLOT(onUsb2SnesDisconnected()));
    connect(usb2snes, &Usb2Snes::deviceListDone, this, [=] (QStringList devList) {
        if (devList.isEmpty() || !devList.at(0).contains("COM"))
        {
            setStatusLabel(tr("Can't find a sd2snes device - retrying in a second"));
            recoTimer.start(1000);
        } else {
            usb2snes->attach(devList.at(0));
            recoTimer.stop();
        }
    }
    );
    usb2snes->connect();
    return true;
}

SendToDialog::~SendToDialog()
{
    delete ui;
}

void SendToDialog::onUsb2SnesStateChanged()
{
    static Usb2Snes::State prevState = Usb2Snes::None;
    Usb2Snes::State uState = usb2snes->state();
    if (uState == Usb2Snes::Connected)
    {
        attachToSD2Snes();
    }
    if (uState == Usb2Snes::Ready)
    {
        if (prevState == Usb2Snes::SendingFile)
        {
            //usb2snes->ls("/");
            setStatusLabel(tr("File transfered"));
            ui->progressBar->setValue(100);
            if (ui->romStartCheckBox->isChecked() && (fileInfos.suffix() == "sfc" || fileInfos.suffix() == "smc"))
                usb2snes->boot(ui->dirLineEdit->text() + "/" + fileInfos.fileName());
            goto endState;
        }
        if (prevState == Usb2Snes::Connected)
        {
            ui->progressBar->setValue(20);
        }
        setStatusLabel(QString(countdownString).arg(SECONDBEFORESTART));
        progressTimer.start(1000);
        usb2snes->setAppName("SendToDialog");
    }
    if (uState == Usb2Snes::SendingFile)
    {
        progressTimer.start(500);
    }
endState:
    prevState = uState;
}

void SendToDialog::onUsb2SnesDisconnected()
{
    progressTimer.stop();
    setStatusLabel(tr("Error with the Usb2Snes application"));
}

void SendToDialog::onTimerTimeout()
{
    static int countdown = SECONDBEFORESTART;
    countdown -= 1;

    if (sendingFile)
    {
        uint pBarValue = 20 + countdown * -1 * progressStep;
        if (pBarValue == 100)
            pBarValue = 99;
        ui->progressBar->setValue(pBarValue);
        return;
    }
    setStatusLabel(QString(countdownString).arg(countdown));
    if (countdown == 0)
    {
        transfertFile();
        progressTimer.stop();
    }
}

void SendToDialog::onRecoTimerTimeout()
{
    attachToSD2Snes();
}

bool SendToDialog::checkForUsb2SnesServer()
{
    QTcpSocket sock;
    sock.connectToHost("localhost", 8080);
    if (sock.waitForConnected(100))
    {
        setStatusLabel(tr("USb2Snes webserver already running"));
        sock.close();
        return true;
    } else {
        QDir appDir(qApp->applicationDirPath());
        wsServer.start(appDir.path() + "/" + "QUsb2Snes.exe", QStringList() << "-nogui");
        return wsServer.waitForStarted(100);
    }
}

void SendToDialog::setStatusLabel(QString message)
{
    qDebug() << message;
    ui->statusLabel->setText(QString("<b>%1</b>").arg(message));
}

void SendToDialog::transfertFile()
{
    QFile fi(fileInfos.absoluteFilePath());
    qDebug() << "Opening file" << fi.open(QIODevice::ReadOnly);
    QByteArray data = fi.readAll();
    sendingFile = true;
    // 6 mb take 60 sec
    int sizeInMb = 1;
    if (data.size() > 1024 * 1024)
        sizeInMb = (data.size() / (1024 * 1024)) + 0.5;
    setStatusLabel(tr("Transfering file - Estimated time is %1 seconds").arg(sizeInMb * 10));
    progressStep = 80 / (sizeInMb * 10);

    usb2snes->sendFile(ui->dirLineEdit->text() + "/" + fileInfos.fileName(), data);
}

void SendToDialog::attachToSD2Snes()
{
    usb2snes->deviceList();
}

void SendToDialog::on_pushButton_clicked()
{
    progressTimer.stop();
    DirChangeDialog diag(usb2snes);
    if (diag.exec())
    {
        ui->dirLineEdit->setText(diag.getDir());
        if (ui->saveDirCheckBox->isChecked())
        {
            settings->setValue(UPDIRKEY, ui->dirLineEdit->text());
        }
    }
    progressTimer.start(1000);
}

void SendToDialog::on_romStartCheckBox_toggled(bool checked)
{
    settings->setValue(EXECAFTERUP, checked);
}
