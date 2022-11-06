
#include <QButtonGroup>
#include "deviceselectorpage.h"
#include "ui_deviceselectorpage.h"

DeviceSelectorPage::DeviceSelectorPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::DeviceSelectorPage)
{
    ui->setupUi(this);
    registerField("SD2SnesDeviceSelected", ui->sd2snesButton);
    registerField("RetroarchDeviceSelected", ui->retroarchButton);
    registerField("SnesClassicDeviceSelected", ui->snesClassicButton);
    registerField("NWADeviceSelected", ui->nwaButton);
    connect(ui->buttonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled), this, [=](QAbstractButton* button, bool checked)
    {
        emit completeChanged();
    });
}

DeviceSelectorPage::~DeviceSelectorPage()
{
    delete ui;
}

bool DeviceSelectorPage::isComplete() const
{
    return ui->buttonGroup->checkedButton() != nullptr;
}

int DeviceSelectorPage::nextId() const
{
    if (ui->sd2snesButton->isChecked())
        return 2;
    if (ui->retroarchButton->isChecked())
        return 3;
    return 42;
}
