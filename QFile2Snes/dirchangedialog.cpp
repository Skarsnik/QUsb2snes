#include "dirchangedialog.h"
#include "ui_dirchangedialog.h"

DirChangeDialog::DirChangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DirChangeDialog)
{
    ui->setupUi(this);
}

DirChangeDialog::~DirChangeDialog()
{
    delete ui;
}
