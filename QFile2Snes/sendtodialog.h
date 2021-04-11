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

#ifndef SENDTODIALOG_H
#define SENDTODIALOG_H

#include <QDialog>
#include <QFileInfo>
#include "usb2snes.h"

namespace Ui {
class SendToDialog;
}

class SendToDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendToDialog(QString filePath, QWidget *parent = nullptr);
    bool init();
    ~SendToDialog();


private slots:
    void    onUsb2SnesStateChanged();
    void    onUsb2SnesDisconnected();
    void    onTimerTimeout();
    void    onRecoTimerTimeout();

    void on_pushButton_clicked();

    void on_romStartCheckBox_toggled(bool checked);

private:
    Ui::SendToDialog *ui;
    QFileInfo   fileInfos;
    Usb2Snes*   usb2snes;
    QProcess    wsServer;
    QSettings*  settings;
    QTimer      progressTimer;
    QTimer      recoTimer;
    QString     countdownString;
    bool        sendingFile;
    int         progressStep;

    bool    checkForUsb2SnesServer();
    void    setStatusLabel(QString message);
    void    transfertFile();
    void    attachToSD2Snes();
};

#endif // SENDTODIALOG_H
