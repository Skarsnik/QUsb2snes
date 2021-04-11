/*
 * Copyright (c) 2021 the QUsb2Snes authors.
 *
 * This file is part of QUsb2Snes.
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

#ifndef DIRCHANGEDIALOG_H
#define DIRCHANGEDIALOG_H

#include <QDialog>
#include "usb2snes.h"
#include "usb2snesfilemodel.h"

namespace Ui {
class DirChangeDialog;
}

class DirChangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirChangeDialog(Usb2Snes* usb, QWidget *parent = nullptr);
    ~DirChangeDialog();
    QString getDir();

private slots:
    void on_listView_doubleClicked(const QModelIndex &index);

private:
    Ui::DirChangeDialog *ui;
    Usb2Snes*   usb2snes;
    Usb2SnesFileModel*  uModel;
};

#endif // DIRCHANGEDIALOG_H
