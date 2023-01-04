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
    static const QRegularExpression twoSlash("\\/\\/+");
    if (!uModel->isDir(index))
        return ;
    QString newPath = uModel->currentDir() + "/" + uModel->data(index, Qt::DisplayRole).toString();
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
