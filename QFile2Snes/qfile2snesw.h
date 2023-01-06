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

#ifndef QFILE2SNESW_H
#define QFILE2SNESW_H

#include <QMainWindow>
#include <QSettings>
#include "myfilesystemmodel.h"
#include "usb2snes.h"
#include "usb2snesfilemodel.h"

namespace Ui {
class QFile2SnesW;
}

class QFile2SnesW : public QMainWindow
{
    Q_OBJECT

public:
    enum State {
      NOTCONNECTED = 0,
      CONNECTING,
      IDLE,
      SENDINDFILE,
      GETTINGFILE,
    };
    explicit QFile2SnesW(QWidget *parent = nullptr);
    ~QFile2SnesW();

private slots:

    void on_localFilesTreeView_doubleClicked(const QModelIndex &index);

    void    onUsb2SnesStateChanged();

    void on_usb2snesListView_doubleClicked(const QModelIndex &index);

    void    onUsb2SnesFileSendProgress(int size);

    void    onSDViewSelectionChanged(const QItemSelection& selected, const QItemSelection &deselected);
    void    onSDViewCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

    void on_renameButton_clicked();

    void on_deleteButton_clicked();
    void onAboutToOverwriteFile(QByteArray data);

    void on_bootButton_clicked();

    void on_resetButton_clicked();

    void on_menuButton_clicked();
    void    onLocalDirectoryLoaded(const QString& path);

    void on_driveComboBox_activated(int arg1);

    void on_patchButton_clicked();

    void on_newDirButton_clicked();

    void on_backToolButton_clicked();

private:
    State               m_state;
    Ui::QFile2SnesW     *ui;
    QSettings*          m_settings;
    Usb2Snes*           usb2snes;
    Usb2SnesFileModel*  usb2snesModel;
    bool                started;
    MyFileSystemModel*  localFileModel;

    void    setLFilepath(QString path);
    void    refreshStatus();
    void    setEnabledSd2SnesUI(bool enabled);

    // QWidget interface
    void updateLocalFileView(QString path);
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // QFILE2SNESW_H
