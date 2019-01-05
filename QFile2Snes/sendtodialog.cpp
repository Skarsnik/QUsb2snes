#include "sendtodialog.h"
#include "ui_sendtodialog.h"

SendToDialog::SendToDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendToDialog)
{
    ui->setupUi(this);
}

SendToDialog::~SendToDialog()
{
    delete ui;
}
