#include <QDebug>

#include <QMessageBox>
#include <QStorageInfo>

#include <QStandardPaths>
#include <QModelIndex>
#include <QDir>
#include <QInputDialog>
#include "qfile2snesw.h"
#include "ui_qfile2snesw.h"
#include "myfilesystemmodel.h"

QFile2SnesW::QFile2SnesW(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QFile2SnesW)
{
    ui->setupUi(this);
    //fillDriveCombo();
    MyFileSystemModel *fileModel = new MyFileSystemModel();
    qDebug() << fileModel->filter();
    fileModel->sort(0);
    fileModel->setFilter(QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::Drives | QDir::AllEntries);
    qDebug() << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    fileModel->setRootPath("/");
    ui->driveComboBox->setModel(fileModel);
    m_settings = new QSettings("skarsnik.nyo.fr", "QFile2Snes");
    QString currentDir;
    if (m_settings->contains("lastLocalDir"))
        currentDir = m_settings->value("lastLocalDir").toString();
    else
        currentDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (m_settings->contains("windowGeometry"))
    {
        restoreGeometry(m_settings->value("windowGeometry").toByteArray());
        restoreState(m_settings->value("windowState").toByteArray());
    }
    ui->localFilesListView->setModel(fileModel);
    ui->localFilesListView->setRootIndex(fileModel->index(currentDir));
    fileModel->filePath(ui->localFilesListView->rootIndex());
    QStorageInfo si(currentDir);
    ui->driveComboBox->setCurrentIndex(ui->driveComboBox->findData(si.rootPath().left(2), Qt::DisplayRole));
    usb2snes = new USB2snes(true);
    usb2snesModel = new Usb2SnesFileModel(usb2snes);
    fileModel->setUsb2Snes(usb2snes);
    ui->usb2snesListView->setModel(usb2snesModel);
    m_state = NOTCONNECTED;
    usb2snes->connect();
    qDebug() << fileModel->mimeTypes();
    ui->transfertProgressBar->setVisible(false);
    ui->infoLabel->setText(tr("Trying to find the SD2Snes device"));
    connect(usb2snes, SIGNAL(stateChanged()), this, SLOT(onUsb2SnesStateChanged()));
    connect(usb2snes, SIGNAL(fileSendProgress(int)), this, SLOT(onUsb2SnesFileSendProgress(int)));
    connect(ui->usb2snesListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSDViewSelectionChanged(const QItemSelection&, const QItemSelection&)));
    connect(ui->usb2snesListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onSDViewCurrentChanged(const QModelIndex&, const QModelIndex&)));
    connect(fileModel, &MyFileSystemModel::aboutToOverwroteFile, this, &QFile2SnesW::onAboutToOverwriteFile);
    connect(fileModel, &MyFileSystemModel::directoryLoaded, this, &QFile2SnesW::onLocalDirectoryLoaded);
    started = false;
}

QFile2SnesW::~QFile2SnesW()
{
    delete ui;
}

void QFile2SnesW::on_localFilesListView_doubleClicked(const QModelIndex &index)
{
    const QFileSystemModel* mod = static_cast<const QFileSystemModel*>(index.model());
    QString path = mod->fileInfo(index).absoluteFilePath();
    ui->localFilesListView->setRootIndex(mod->index(path));
    ui->currentPathLabel->setText(path.left(3) + path.mid(3).right(100));

}

void    QFile2SnesW::refreshStatus()
{
    USB2snes::DeviceInfo infos = usb2snes->infos();
    ui->infoLabel->setText(QString(tr("Firmware version : %1 - Rom Playing : %2")).arg(infos.firmwareVersion, infos.romPlaying));
}

void QFile2SnesW::onUsb2SnesStateChanged()
{
    if (usb2snes->state() == USB2snes::Ready)
    {
        if (m_state == NOTCONNECTED)
        {
            m_state = IDLE;
            usb2snes->setAppName("QFile2Snes");
            listAndAttach();
            refreshStatus();
        }
        if (m_state == SENDINDFILE || m_state == GETTINGFILE)
        {
            qDebug() << "requesting info";
            usb2snes->infos();
            ui->transfertProgressBar->setValue(100);
            m_state = IDLE;
        }
        usb2snesModel->setPath(usb2snesModel->currentDir());
        //ui->transfertProgressBar->setEnabled(false);
    }
    if (usb2snes->state() == USB2snes::SendingFile)
    {
        qDebug() << "Sending file";
        ui->transfertProgressBar->setVisible(true);
        ui->transfertProgressBar->setValue(99);
        m_state = SENDINDFILE;
    }
    if (usb2snes->state() == USB2snes::ReceivingFile)
    {
        qDebug() << "Receiving file";
        ui->transfertProgressBar->setVisible(true);
        ui->transfertProgressBar->setValue(99);
        m_state = GETTINGFILE;
    }
}

bool QFile2SnesW::listAndAttach()
{
    QStringList devices = usb2snes->deviceList();
    /*if (devices.size() != 0)
        usb2snes->usePort(devices.at(0));*/
    ui->deviceComboBox->clear();
    foreach(QString dev, devices)
    {
        ui->deviceComboBox->addItem(dev);
    }
    return false;
}

void QFile2SnesW::on_usb2snesListView_doubleClicked(const QModelIndex &index)
{
    if (!usb2snesModel->isDir(index))
        return ;
    QString newPath = usb2snesModel->currentDir() + "/" + usb2snesModel->data(index, Qt::DisplayRole).toString();
    qDebug() << "Unormalized path : " << newPath;
    QRegExp twoSlash("\\/\\/+");
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
    MyFileSystemModel* model = static_cast<MyFileSystemModel*>(ui->localFilesListView->model());
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

void QFile2SnesW::on_driveComboBox_activated(const QString &arg1)
{
    QFileSystemModel* mod = static_cast<QFileSystemModel*> (ui->driveComboBox->model());
    QString path = mod->fileInfo(ui->driveComboBox->view()->currentIndex()).absoluteFilePath();
    ui->localFilesListView->setRootIndex(mod->index(path));
    mod->sort(0);
}


void QFile2SnesW::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_settings->setValue("windowState", saveState());
    m_settings->setValue("windowGeometry", saveGeometry());
    const QFileSystemModel* mod = static_cast<const QFileSystemModel*>(ui->localFilesListView->model());
    m_settings->setValue("lastLocalDir", mod->fileInfo(ui->localFilesListView->rootIndex()).absoluteFilePath());
}
