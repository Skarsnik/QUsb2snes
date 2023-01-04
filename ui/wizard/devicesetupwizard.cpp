#include "devicesetupwizard.h"
#include <ui/wizard/retroarchpage.h>
#include "ui_devicesetupwizard.h"
#include <ui/wizard/lastpage.h>
#include <ui/wizard/sd2snespage.h>
#include <ui/wizard/deviceselectorpage.h>
#include <QMessageBox>

DeviceSetupWizard::DeviceSetupWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::DeviceSetupWizard)
{
    ui->setupUi(this);
    setPage(1, new DeviceSelectorPage(this));
    setPage(2, new Sd2SnesPage(this));
    setPage(3, new RetroArchPage(this));
    setPage(42, new LastPage(this));
}

DeviceSetupWizard::~DeviceSetupWizard()
{
    delete ui;
}

DeviceSetupWizard::DeviceSelected DeviceSetupWizard::deviceSelected() const
{
    if (field("SD2SnesDeviceSelected").toBool())
        return SD2SNES;
    if (field("RetroarchDeviceSelected").toBool())
        return RETROARCH;
    if (field("SnesClassicDeviceSelected").toBool())
        return SNESCLASSIC;
    if (field("NWADeviceSelected").toBool())
        return NWA;
}

bool DeviceSetupWizard::contextMenu() const
{
    return field("Sd2SnesContextMenu").toBool();
}

bool DeviceSetupWizard::showQuitMessage()
{
    auto question = QMessageBox::question(this, tr("Abord configurtion"),
      tr("Closing the wizard will leave you with an unconfigured QUsb2snes. You can still activate support later from the system tray device menu but you will not be guided"));
    return question == QMessageBox::Yes;
}

void DeviceSetupWizard::closeEvent(QCloseEvent *event)
{
    if (showQuitMessage())
    {
        event->accept();
    } else {
        event->ignore();
    }
}

void DeviceSetupWizard::reject()
{
    if (showQuitMessage())
    {
        QWizard::reject();
    }
}

