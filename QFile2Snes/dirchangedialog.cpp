#include "dirchangedialog.h"
#include "ui_dirchangedialog.h"


DirChangeDialog::DirChangeDialog(Usb2Snes *usb, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DirChangeDialog)
{
    ui->setupUi(this);
    usb2snes = usb;
    uModel = new Usb2SnesFileModel(usb);
    uModel->setDirOnly(true);
    ui->listView->setModel(uModel);
    uModel->setPath("/");
}

DirChangeDialog::~DirChangeDialog()
{
    delete ui;
}

QString DirChangeDialog::getDir()
{
    return uModel->currentDir();
}

void DirChangeDialog::on_listView_doubleClicked(const QModelIndex &index)
{
    if (!uModel->isDir(index))
        return ;
    QString newPath = uModel->currentDir() + "/" + uModel->data(index, Qt::DisplayRole).toString();
    QRegExp twoSlash("\\/\\/+");
    newPath.replace(twoSlash, "/");
    QUrl baseUrl("");
    newPath = baseUrl.resolved(QUrl(newPath)).path();
    if (newPath.endsWith("/"))
    {
        newPath.chop(1);
    }
    uModel->setPath(newPath);
    ui->lineEdit->setText(newPath);
}
