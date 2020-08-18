#include "tempdeviceselector.h"
#include "ui_tempdeviceselector.h"

TempDeviceSelector::TempDeviceSelector(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TempDeviceSelector)
{
    ui->setupUi(this);
}

TempDeviceSelector::~TempDeviceSelector()
{
    delete ui;
}

void TempDeviceSelector::on_buttonBox_accepted()
{
    if (ui->sd2snesRadioButton->isChecked())
        devices.append("SD2SNES");
    if (ui->luaRadioButton->isChecked())
        devices.append("LUA");
    if (ui->classicRradioButton->isChecked())
        devices.append("CLASSIC");
    if (ui->retroarchRadioButton->isChecked())
        devices.append("RETROARCH");
    accept();
}
