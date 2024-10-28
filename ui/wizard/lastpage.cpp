#include "lastpage.h"
#include "ui_lastpage.h"

#include <QSysInfo>
#include <QTimer>

LastPage::LastPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::LastPage)
{
    ui->setupUi(this);
    canNext = false;
    registerField("SendTelemetryData", ui->telemetryCheckBox);
    QTimer::singleShot(3000, this, [=]
    {
        canNext = true;
        emit completeChanged();
    });
}

void LastPage::initializePage()
{
    QString device;
    if (field("SD2SnesDeviceSelected").toBool())
        device = "SD2SNES";
    if (field("RetroarchDeviceSelected").toBool())
        device = "RETROARCH";
    if (field("SnesClassicDeviceSelected").toBool())
        device = "SNESCLASSIC";
    if (field("NWADeviceSelected").toBool())
        device = "NWA";
    ui->telemetryLabel->setText(QString(tr("Data that will be sent :\n QUsb2Snes' version : %1 - Device selected : %3\n System : %2")).arg(qApp->applicationVersion(), QSysInfo::prettyProductName(), device));
}

LastPage::~LastPage()
{
    delete ui;
}

bool LastPage::isComplete() const
{
    return canNext;
}
