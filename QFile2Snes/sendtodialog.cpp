#include "dirchangedialog.h"
#include "sendtodialog.h"
#include "ui_sendtodialog.h"
#include <QDebug>
#include <QMessageBox>

#define UPDIRKEY    "UploadDir"
#define SECONDBEFORESTART 3


SendToDialog::SendToDialog(QString filePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendToDialog)
{
    ui->setupUi(this);
    fileInfos.setFile(filePath);
    ui->filenameLabel->setText(fileInfos.fileName());
    qDebug() << fileInfos.filePath();
    usb2snes = nullptr;
    setWindowTitle(QString(tr("Send %1 to SD2SNES").arg(fileInfos.fileName())));

    settings = new QSettings("skarsnik.nyo.fr", "QFile2Snes");
    if (!settings->contains(UPDIRKEY))
    {
        settings->setValue(UPDIRKEY, "/");
    }
    ui->dirLineEdit->setText(settings->value(UPDIRKEY).toString());

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
    usb2snes = new USB2snes(false);
    connect(usb2snes, SIGNAL(stateChanged()), this, SLOT(onUsb2SnesStateChanged()));
    connect(usb2snes, SIGNAL(disconnect()), this, SLOT(onUsb2SnesDisconnected()));
    usb2snes->connect();
    return true;
}

SendToDialog::~SendToDialog()
{
    delete ui;
}

void SendToDialog::onUsb2SnesStateChanged()
{
    static USB2snes::State prevState = USB2snes::None;
    USB2snes::State uState = usb2snes->state();
    if (uState == USB2snes::Connected)
    {
        attachToSD2Snes();
    }
    if (uState == USB2snes::Ready)
    {
        if (prevState == USB2snes::SendingFile)
        {
            usb2snes->ls("/");
            setStatusLabel(tr("File transfered"));
            ui->progressBar->setValue(100);
            if (ui->romStartCheckBox->isChecked())
                usb2snes->boot(ui->dirLineEdit->text() + "/" + fileInfos.fileName());
            goto endState;
        }
        if (prevState == USB2snes::Connected)
        {
            ui->progressBar->setValue(20);
        }
        setStatusLabel(QString(countdownString).arg(SECONDBEFORESTART));
        progressTimer.start(1000);
        usb2snes->setAppName("SendToDialog");
    }
    if (uState == USB2snes::SendingFile)
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
        appDir.cdUp();
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
    fi.open(QIODevice::ReadOnly);
    QByteArray data = fi.readAll();
    sendingFile = true;
    // 6 mb take 60 sec
    int sizeInMb = (data.size() / (1024 * 1024)) + 0.5;
    setStatusLabel(tr("Transfering file - Estimated time is %1 seconds").arg(sizeInMb * 10));
    progressStep = 80 / (sizeInMb * 10);

    usb2snes->sendFile(ui->dirLineEdit->text() + "/" + fileInfos.fileName(), data);

}

void SendToDialog::attachToSD2Snes()
{
    QStringList devList = usb2snes->deviceList();
    if (devList.isEmpty() || !devList.at(0).contains("COM"))
    {
        setStatusLabel(tr("Can't find a sd2snes device - retrying in a second"));
        recoTimer.start(1000);
    } else {
        usb2snes->attach(devList.at(0));
        recoTimer.stop();
    }
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
