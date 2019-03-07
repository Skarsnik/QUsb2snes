#include "myfilesystemmodel.h"

MyFileSystemModel::MyFileSystemModel()
{

}


bool MyFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    qDebug() << "Mfile model Drop action" << row << column << parent << data->formats();
    const QString& sd2snesFilePath = data->urls().at(0).toString();
    qDebug() << "Path : " << data->urls().at(0) << sd2snesFilePath;
    m_fileData.clear();
    m_sizeReceived = 0;
    m_fileSize = 0;
    m_filePath = this->fileInfo(parent).absoluteFilePath() + "/" + data->urls().at(0).fileName();
    m_fileSize = usb2snes->getFile(sd2snesFilePath);
    qDebug() << m_fileSize;
    return false;
}

Qt::ItemFlags MyFileSystemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QFileSystemModel::flags(index);

    if (!index.isValid())
        return defaultFlags;
    const QFileInfo& fileInfo = this->fileInfo(index);
    if (fileInfo.isDir())
        return defaultFlags | Qt::ItemIsDropEnabled;
    if (fileInfo.isFile())
        return defaultFlags | Qt::ItemIsDragEnabled;
    return defaultFlags;
}

void MyFileSystemModel::setUsb2Snes(USB2snes *usb)
{
    usb2snes = usb;
    connect(usb2snes,  &USB2snes::getFileDataGet, this, &MyFileSystemModel::OnUsbFileData);
}

const QString &MyFileSystemModel::getFilePath() const
{
    return m_filePath;
}

Qt::DropActions MyFileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

void MyFileSystemModel::OnUsbFileData(QByteArray data)
{
    qDebug() << "<<B" << data.size();
    m_fileData.append(data);
    m_sizeReceived += data.size();
    qDebug() << m_sizeReceived;
    if (m_sizeReceived == m_fileSize)
    {
        if (QFileInfo::exists(m_filePath))
        {
            emit aboutToOverwroteFile(m_fileData);
            return ;
        }
        qDebug() << "Creating " << m_filePath << "of " << m_fileData.size();
        QFile file(m_filePath);
        file.open(QIODevice::WriteOnly | QIODevice::NewOnly);
        file.write(m_fileData);
        file.close();
    }
}
