#include <QSerialPortInfo>
#include <QLoggingCategory>
#include <QSerialPort>
#include <QTimer>
#include <QFileInfo>


Q_LOGGING_CATEGORY(log_wizsd2snes, "Wizard SD2SNES")
#define sDebug() qCDebug(log_wizsd2snes)
#define sInfo() qCInfo(log_wizsd2snes)


#include "sd2snespage.h"
#include "ui_sd2snespage.h"


Sd2SnesPage::Sd2SnesPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::Sd2SnesPage)
{
    ui->setupUi(this);
    deviceFound = false;
    sd2snesDevice = nullptr;
    sd2snesInfoDone = false;
    ui->contextMenuCheckBox->setChecked(false);
    registerField("Sd2SnesContextMenu", ui->contextMenuCheckBox);
    ui->contextMenuCheckBox->setVisible(false);
#ifdef Q_OS_WIN
    ui->contextMenuCheckBox->setChecked(true);
    ui->contextMenuCheckBox->setVisible(true);
#endif
}

Sd2SnesPage::~Sd2SnesPage()
{
    delete ui;
}

void Sd2SnesPage::refreshCOMPort()
{
    emit completeChanged();
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    deviceFound = false;
    ui->comStatusLabel->setText(tr("Trying to find the Sd2Snes or FxPak pro"));
    ui->refreshButton->setEnabled(false);
    foreach (QSerialPortInfo usbinfo, sinfos)
    {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber();
        if (usbinfo.serialNumber() == "DEMO00000000")
        {
            bool isBusy = false;
            QSerialPort sPort(usbinfo);
            isBusy = !sPort.open(QIODevice::ReadWrite);
            QString errorString = sPort.errorString();
            auto error = sPort.error();
            sPort.close();
            deviceFound = true;
            if (isBusy)
            {
                if (error == QSerialPort::PermissionError)
                    errorString = "You don't have the permission on the device or the device is used by another program (did you run QUsb2Snes twice?)";
#if defined Q_OS_UNIX || defined Q_OS_MAC
                QFileInfo fi(usbinfo.systemLocation());
                if (!fi.permission(QFile::WriteUser | QFile::WriteGroup))
                    errorString = "You don't have permission to use the serial device, consider adding yourself to the correct group (<i>uucp</i> or <i>dialout</i> for example)";
#endif
                ui->comStatusLabel->setText(QString(tr("SD2Snes/FXPak pro found on port %1 but can't use the device. %2")).arg(usbinfo.portName(), errorString));
            } else {
                if (sd2snesDevice != nullptr)
                {
                    sd2snesDevice->deleteLater();
                }
                sd2snesDevice = new SD2SnesDevice(usbinfo.portName());
                sd2snesGetInfos();
                ui->comStatusLabel->setText(QString(tr("SD2Snes found on port %1")).arg(usbinfo.portName()));
            }
            return ;
        }
    }
    ui->refreshButton->setEnabled(true);
    ui->comStatusLabel->setText(tr("No serial device corresponding to a SD2Snes/FXPak pro found"));
}

void Sd2SnesPage::sd2snesGetInfos()
{
    if (sd2snesDevice == nullptr)
        return ;
    sd2snesDevice->open();
    ui->firmwareStatusLabel->setText(tr("Connecting to the device and trying to find the firmware information"));
    sd2snesDevice->infoCommand();
    sd2snesInfoDone = false;
    connect(sd2snesDevice, &SD2SnesDevice::commandFinished, this, [=] {
        auto info = sd2snesDevice->parseInfo(sd2snesDevice->dataRead);
        ui->firmwareStatusLabel->setText(QString(tr("Info command : %1\nFirmware version : %2\nCurrently playing :%3")).arg(info.deviceName, info.version, info.romPlaying));
        sd2snesInfoDone = true;
        sd2snesDevice->close();
        ui->refreshButton->setEnabled(true);
        emit completeChanged();
    });
    QTimer::singleShot(2000, this, [=] {
        if (sd2snesDevice->state() == ADevice::BUSY)
        {
            ui->firmwareStatusLabel->setText(tr("The SD2Snes device failed to reply to info in time. Try power off/on your SNES and try again"));
            sd2snesDevice->close();
            ui->refreshButton->setEnabled(true);
            emit completeChanged();
        }
    });
}

void Sd2SnesPage::initializePage()
{
    refreshCOMPort();
}

int Sd2SnesPage::nextId() const
{
    return 42;
}

bool Sd2SnesPage::isComplete() const
{
    return sd2snesInfoDone == true;
}

void Sd2SnesPage::on_refreshButton_clicked()
{
    refreshCOMPort();
}

