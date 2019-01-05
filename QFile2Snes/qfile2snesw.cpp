#include "qfile2snesw.h"
#include "ui_qfile2snesw.h"
#include <QDebug>

#include <QStorageInfo>
#include <QFileSystemModel>
#include <QStandardPaths>
#include <QModelIndex>
#include <QDir>
#include <QInputDialog>

QFile2SnesW::QFile2SnesW(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QFile2SnesW)
{
    ui->setupUi(this);
    //fillDriveCombo();
    QFileSystemModel *fileModel = new QFileSystemModel();
    qDebug() << fileModel->filter();
    fileModel->sort(0);
    fileModel->setFilter(QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::Drives | QDir::AllEntries);
    qDebug() << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    fileModel->setRootPath("/");
    ui->driveComboBox->setModel(fileModel);
    ui->localFilesListView->setModel(fileModel);
    ui->localFilesListView->setRootIndex(fileModel->index(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));    
    usb2snes = new USB2snes(true);
    usb2snesModel = new Usb2SnesFileModel(usb2snes);
    ui->usb2snesListView->setModel(usb2snesModel);
    m_state = NOTCONNECTED;
    usb2snes->connect();
    qDebug() << fileModel->mimeTypes();
    connect(usb2snes, SIGNAL(stateChanged()), this, SLOT(onUsb2SnesStateChanged()));
    connect(usb2snes, SIGNAL(fileSendProgress(int)), this, SLOT(onUsb2SnesFileSendProgress(int)));
    connect(ui->usb2snesListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSDViewSelectionChanged(const QItemSelection&, const QItemSelection&)));
    connect(ui->usb2snesListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onSDViewCurrentChanged(const QModelIndex&, const QModelIndex&)));
}

QFile2SnesW::~QFile2SnesW()
{
    delete ui;
}

void QFile2SnesW::on_driveComboBox_currentIndexChanged(int index)
{
    QFileSystemModel* mod = static_cast<QFileSystemModel*> (ui->driveComboBox->model());
    QString path = mod->fileInfo(ui->driveComboBox->view()->currentIndex()).absoluteFilePath();
    ui->localFilesListView->setRootIndex(mod->index(path));
    mod->sort(0);
}

void QFile2SnesW::on_localFilesListView_doubleClicked(const QModelIndex &index)
{
    const QFileSystemModel* mod = static_cast<const QFileSystemModel*>(index.model());
    QString path = mod->fileInfo(index).absoluteFilePath();
    ui->localFilesListView->setRootIndex(mod->index(path));
}

void QFile2SnesW::onUsb2SnesStateChanged()
{
    if (usb2snes->state() == USB2snes::Ready)
    {
        if (m_state == NOTCONNECTED)
        {
            m_state = IDLE;
            usb2snes->setAppName("QFile2Snes");
        }
        if (m_state == SENDINDFILE)
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
        ui->transfertProgressBar->setValue(99);
        m_state = SENDINDFILE;
    }
}

bool QFile2SnesW::listAndAttach()
{
    QStringList devices = usb2snes->deviceList();
    /*if (devices.size() != 0)
        usb2snes->usePort(devices.at(0));*/
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
    qDebug() << "Selection changed";
    ui->renameButton->setEnabled(!selected.isEmpty());
    ui->deleteButton->setEnabled(!selected.isEmpty());
}

void QFile2SnesW::onSDViewCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    qDebug() << "current Changed";
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
