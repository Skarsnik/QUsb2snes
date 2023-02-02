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

#include <QDebug>

#include <QFileDialog>
#include <QMessageBox>
#include <QStorageInfo>

#include <QStandardPaths>
#include <QModelIndex>
#include <QDir>
#include <QInputDialog>
#include <QRegularExpression>
#include "qfile2snesw.h"
#include "ui_qfile2snesw.h"
#include "myfilesystemmodel.h"

QFile2SnesW::QFile2SnesW(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QFile2SnesW)
{
    ui->setupUi(this);
    //fillDriveCombo();
    localFileModel = new MyFileSystemModel();
    qDebug() << localFileModel->filter();
    localFileModel->sort(0);
    localFileModel->setFilter(QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::Drives | QDir::AllEntries | QDir::NoDotAndDotDot);
    qDebug() << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    localFileModel->setRootPath("/");

    ui->driveComboBox->setModel(localFileModel);
    m_settings = new QSettings("skarsnik.nyo.fr", "QFile2Snes");
    ui->localFilesTreeView->setModel(localFileModel);
    QString currentDir;
    if (m_settings->contains("lastLocalDir"))
        currentDir = m_settings->value("lastLocalDir").toString();
    else
        currentDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (m_settings->contains("windowGeometry"))
    {
        restoreGeometry(m_settings->value("windowGeometry").toByteArray());
        restoreState(m_settings->value("windowState").toByteArray());
        ui->localFilesTreeView->header()->restoreState(m_settings->value("localfileheaderState").toByteArray());
    }
    localFileModel->setRootPath(currentDir);
    ui->localFilesTreeView->setRootIndex(localFileModel->index(currentDir));
    ui->localFilesTreeView->setSortingEnabled(true);
    QStorageInfo si(currentDir);
    ui->driveComboBox->setCurrentIndex(ui->driveComboBox->findData(si.rootPath().left(2), Qt::DisplayRole));
    usb2snes = new Usb2Snes(false);
    usb2snesModel = new Usb2SnesFileModel(usb2snes);
    localFileModel->setUsb2Snes(usb2snes);
    ui->usb2snesListView->setModel(usb2snesModel);
    m_state = NOTCONNECTED;
    qDebug() << localFileModel->mimeTypes();
    ui->transfertProgressBar->setVisible(false);
    ui->infoLabel->setText(tr("Trying to find the SD2Snes device"));
    connect(usb2snes, &Usb2Snes::stateChanged, this, &QFile2SnesW::onUsb2SnesStateChanged);
    connect(usb2snes, &Usb2Snes::fileSendProgress, this, &QFile2SnesW::onUsb2SnesFileSendProgress);
    connect(ui->usb2snesListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QFile2SnesW::onSDViewSelectionChanged);
    connect(ui->usb2snesListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &QFile2SnesW::onSDViewCurrentChanged);
    connect(localFileModel, &MyFileSystemModel::aboutToOverwroteFile, this, &QFile2SnesW::onAboutToOverwriteFile);
    connect(localFileModel, &MyFileSystemModel::directoryLoaded, this, &QFile2SnesW::onLocalDirectoryLoaded);
    started = false;
    ui->backToolButton->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
    ui->newDirButton->setIcon(style()->standardPixmap(QStyle::SP_FileDialogNewFolder));
    setEnabledSd2SnesUI(false);


    connect(usb2snes, &Usb2Snes::connected, this, [=]() {
        usb2snes->setAppName("QFile2Snes");
        ui->infoLabel->setText(tr("Connected to usb2snes server, trying to find a suitable device"));
        usb2snes->deviceList();
    });
    connect(usb2snes, &Usb2Snes::disconnected, this, [=]() {
        ui->sd2snesLabel->setText(tr("Disconnected, trying to reconnect in 1 sec"));
        QTimer::singleShot(1000, this, [=] {
            usb2snes->connect();
        });
    });
    connect(usb2snes, &Usb2Snes::deviceListDone, this, [=] (QStringList devices) {
        ui->deviceComboBox->clear();
        foreach(QString dev, devices)
        {
            ui->deviceComboBox->addItem(dev);
        }
        if (!devices.empty())
        {
            usb2snes->attach(devices.at(0));
            refreshStatus();
        } else {
            QTimer::singleShot(1000, this, [=] {
                if (usb2snes->state() == Usb2Snes::Connected)
                    usb2snes->deviceList();
            });
        }
    });
    connect(usb2snes, &Usb2Snes::infoDone, this, [=] (Usb2Snes::DeviceInfo infos) {
        ui->infoLabel->setText(QString(tr("Firmware version : %1 - Rom Playing : %2")).arg(infos.firmwareVersion, infos.romPlaying));
        if (infos.flags.contains("NO_FILE_CMD"))
        {
            setEnabledSd2SnesUI(false);
            QMessageBox::information(this, tr("Device error"), tr("The device does not support file operation"));
        } else {
            setEnabledSd2SnesUI(true);
            usb2snesModel->setPath(usb2snesModel->currentDir());
        }
    });
    QTimer::singleShot(0, this, [=] {
            usb2snes->connect();
    });
}

QFile2SnesW::~QFile2SnesW()
{
    delete ui;
}

void QFile2SnesW::updateLocalFileView(QString path)
{
    ui->currentPathLabel->setText(path.left(3) + path.mid(3).right(100));
    localFileModel->setRootPath(path);
    ui->localFilesTreeView->setRootIndex(localFileModel->index(path));
}

void QFile2SnesW::on_localFilesTreeView_doubleClicked(const QModelIndex &index)
{
    const QFileSystemModel* mod = static_cast<const QFileSystemModel*>(index.model());
    QString path = mod->fileInfo(index).absoluteFilePath();
    updateLocalFileView(path);
}

void    QFile2SnesW::refreshStatus()
{
    usb2snes->infos();
}

void QFile2SnesW::setEnabledSd2SnesUI(bool enabled)
{
    ui->usb2snesListView->setEnabled(enabled);
    ui->menuButton->setEnabled(enabled);
    ui->deleteButton->setEnabled(enabled);
    ui->newDirButton->setEnabled(enabled);
    ui->renameButton->setEnabled(enabled);
    ui->bootButton->setEnabled(enabled);
}

void QFile2SnesW::onUsb2SnesStateChanged()
{
    qDebug() << "State changed" << usb2snes->state();
    if (usb2snes->state() == Usb2Snes::SendingFile)
    {
        qDebug() << "Sending file";
        ui->transfertProgressBar->setVisible(true);
        ui->transfertProgressBar->setInvertedAppearance(false);
        ui->transfertProgressBar->setValue(99);
        usb2snes->queueInfos();
        m_state = SENDINDFILE;
    }
    if (usb2snes->state() == Usb2Snes::ReceivingFile)
    {
        qDebug() << "Receiving file";
        ui->transfertProgressBar->setVisible(true);
        ui->transfertProgressBar->setInvertedAppearance(true);
        ui->transfertProgressBar->setValue(99);
        //usb2snes->queueInfos();
        m_state = GETTINGFILE;
    }
    if (usb2snes->state() == Usb2Snes::Ready && (m_state == GETTINGFILE || m_state == SENDINDFILE))
    {
        if (m_state == SENDINDFILE)
        {
            usb2snes->infos();
        }
        m_state = IDLE;
        ui->transfertProgressBar->setValue(100);
        ui->transfertProgressBar->setEnabled(false);

    }

}


void QFile2SnesW::on_usb2snesListView_doubleClicked(const QModelIndex &index)
{
    static const QRegularExpression twoSlash("\\/\\+");
    if (!usb2snesModel->isDir(index))
        return ;
    QString newPath = usb2snesModel->currentDir() + "/" + usb2snesModel->data(index, Qt::DisplayRole).toString();
    qDebug() << "Unormalized path : " << newPath;
    //QRegExp twoSlash("\\/\\/+");
    newPath.replace(twoSlash, "/");
    qDebug() << "REgexed path : " << newPath;
    QUrl baseUrl("");
    newPath = baseUrl.resolved(QUrl(newPath)).path();
    qDebug() << "New path : " << newPath;
    if (newPath.endsWith("/"))
    {
        qDebug() << "trailing /";
        newPath.chop(1);
    }
    usb2snesModel->setPath(newPath);
    ui->sd2snesLabel->setText(QString(tr("SD2Snes directory: %1")).arg(newPath));
    ui->usb2snesListView->clearSelection();
    ui->usb2snesListView->selectionModel()->clearSelection();
    ui->renameButton->setEnabled(false);
    ui->deleteButton->setEnabled(false);
}

void QFile2SnesW::onUsb2SnesFileSendProgress(int size)
{
    return ; // This is useless due to how the protocol work
    qDebug() << "Progressing transfert" << size;
    ui->transfertProgressBar->setValue((size / usb2snes->fileDataSize()) * 100);
}

void QFile2SnesW::onSDViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    qDebug() << "Selection changed";
    ui->renameButton->setEnabled(!selected.isEmpty());
    ui->deleteButton->setEnabled(!selected.isEmpty());
}

void QFile2SnesW::onSDViewCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    Q_UNUSED(current)
    qDebug() << "current Changed";
    ui->bootButton->setEnabled(false);
    QString fileName = usb2snesModel->data(ui->usb2snesListView->currentIndex()).toString();
    if (fileName.right(4) == ".smc" || fileName.right(4) == ".sfc")
        ui->bootButton->setEnabled(true);
}

void QFile2SnesW::on_renameButton_clicked()
{
    bool    ok;
    QString currentName = usb2snesModel->data(ui->usb2snesListView->currentIndex()).toString();
    QString newName = QInputDialog::getText(this, tr("Rename dialog"), tr("New filename"), QLineEdit::Normal, currentName, &ok);
    if (!ok)
        return;
    //newName = usb2snesModel->currentDir() + "/" + newName;
    currentName = usb2snesModel->currentDir() + "/" + currentName;
    qDebug() << currentName << newName;
    usb2snes->renameFile(currentName, newName);
    usb2snesModel->setPath(usb2snesModel->currentDir());
}

void QFile2SnesW::on_deleteButton_clicked()
{
    QString currentName = usb2snesModel->data(ui->usb2snesListView->currentIndex()).toString();
    usb2snes->deleteFile(usb2snesModel->currentDir() + "/" + currentName);
    usb2snesModel->setPath(usb2snesModel->currentDir());
}

void QFile2SnesW::onAboutToOverwriteFile(QByteArray data)
{
    MyFileSystemModel* model = static_cast<MyFileSystemModel*>(ui->localFilesTreeView->model());
    QString fileName = model->getFilePath();
    int ret = QMessageBox::warning(this, tr("Writing over an existing file"), QString(tr("You are about to overwrite %1 do you want to continue?")).arg(fileName), QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
    {
        qDebug() << "Creating " << fileName << "of " << data.size();
        QFile file(model->getFilePath());
        file.open(QIODevice::WriteOnly);
        file.write(data);
        file.close();
    }
}

void QFile2SnesW::on_bootButton_clicked()
{
    QString currentName = usb2snesModel->data(ui->usb2snesListView->currentIndex()).toString();
    usb2snes->boot(usb2snesModel->currentDir() + "/" + currentName);
    refreshStatus();
}

void QFile2SnesW::on_resetButton_clicked()
{
    usb2snes->reset();
}

void QFile2SnesW::on_menuButton_clicked()
{
    usb2snes->menu();
    refreshStatus();
}

void QFile2SnesW::onLocalDirectoryLoaded(const QString& path)
{
    ui->currentPathLabel->setText(path.left(3) + path.mid(3).right(100));
}

void QFile2SnesW::on_driveComboBox_activated(int arg1)
{
    Q_UNUSED(arg1)
    QFileSystemModel* mod = static_cast<QFileSystemModel*> (ui->driveComboBox->model());
    QString path = mod->fileInfo(ui->driveComboBox->view()->currentIndex()).absoluteFilePath();
    ui->localFilesTreeView->setRootIndex(mod->index(path));
    mod->sort(0);
}


void QFile2SnesW::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_settings->setValue("windowState", saveState());
    m_settings->setValue("windowGeometry", saveGeometry());
    const QFileSystemModel* mod = static_cast<const QFileSystemModel*>(ui->localFilesTreeView->model());
    m_settings->setValue("lastLocalDir", mod->fileInfo(ui->localFilesTreeView->rootIndex()).absoluteFilePath());
    m_settings->setValue("localfileheaderState", ui->localFilesTreeView->header()->saveState());
}

void QFile2SnesW::on_patchButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose an IPS File"),
                                                      "",
                                                      tr("IPS File (*.ips)"));
    if (fileName.isEmpty())
        return ;
    if (usb2snes->patchROM(fileName))
        ui->statusBar->showMessage(tr("Patch applied succesfully"));
    else {
        ui->statusBar->showMessage(tr("Error when applying a patch"));
    }
}

void QFile2SnesW::on_newDirButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, QString(tr("Create new directory in %1").arg(usb2snesModel->currentDir())),
                                              tr("Directory name:"), QLineEdit::Normal,
                                              "New Folder", &ok);
    if (ok && !text.isEmpty())
    {
        usb2snesModel->setPath(usb2snesModel->currentDir());
        usb2snes->mkdir(usb2snesModel->currentDir() + "/" + text);
        usb2snesModel->setPath(usb2snesModel->currentDir());
    }
}

void QFile2SnesW::on_backToolButton_clicked()
{
    QFileSystemModel* mod = static_cast<QFileSystemModel*> (ui->localFilesTreeView->model());
    qDebug() << mod->rootPath();
    QDir plop(mod->rootPath());
    plop.cdUp();
    qDebug() << plop.absolutePath();
    updateLocalFileView(plop.absolutePath());
}

