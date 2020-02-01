#include "usb2snesfilemodel.h"

#include <QFileIconProvider>
#include <QDebug>
#include <QFile>

Usb2SnesFileModel::Usb2SnesFileModel(Usb2Snes *usb, QObject *parent)
    : QAbstractListModel(parent)
{
        usb2snes = usb;
        m_currentDir = "";
        dirOnly = false;
}


int Usb2SnesFileModel::rowCount(const QModelIndex &parent) const
{
    return fileInfos.size();
}

QVariant Usb2SnesFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() && index.row() >= fileInfos.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    {
        return QVariant(fileInfos.at(index.row()).name);
    }
    case Qt::DecorationRole:
    {
        QFileIconProvider provid;
        if (fileInfos.at(index.row()).dir)
            return provid.icon((QFileIconProvider::Folder));
        else
            return provid.icon((QFileIconProvider::File));
    }

    }
    return QVariant();
}

void Usb2SnesFileModel::setPath(QString path)
{
    m_currentDir = path;
    QList<Usb2Snes::FileInfo> li = usb2snes->ls(path);
    if (dirOnly)
    {
        fileInfos.clear();
        foreach(Usb2Snes::FileInfo fi, li)
        {
            if (fi.dir)
                fileInfos.append(fi);
        }
    } else {
        fileInfos = li;
    }
    emit endResetModel();
}

QString Usb2SnesFileModel::currentDir() const
{
    return m_currentDir;
}

bool Usb2SnesFileModel::isDir(const QModelIndex &index) const
{
    return fileInfos.at(index.row()).dir;
}

void Usb2SnesFileModel::setDirOnly(bool)
{
    dirOnly = true;
}


QVariant Usb2SnesFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
        return QVariant(m_currentDir);
    return QVariant();
}


QStringList Usb2SnesFileModel::mimeTypes() const
{
    return QStringList() << "text/uri-list";
}

QMimeData *Usb2SnesFileModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* mData = new QMimeData;
    QString text;
    QList<QUrl> urls;
    foreach(QModelIndex idx, indexes)
    {
        urls.append(QUrl(m_currentDir + "/" + fileInfos.at(idx.row()).name));
    }
    mData->setUrls(urls);
    return mData;
}

bool Usb2SnesFileModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    qDebug() << "Drop action" << row << column << parent << data->formats();
    QString dest = m_currentDir;
    if (parent.isValid())
    {
        if (fileInfos.at(parent.row()).dir)
            dest = m_currentDir + "/" + fileInfos.at(parent.row()).name;
    }
    if (!data->hasUrls())
        return false;
    qDebug() << data->urls().at(0);
    QFile fi(data->urls().at(0).toLocalFile());
    QFileInfo fileInfo(fi);
    if (fileInfo.isDir())
        return false;

    fi.open(QIODevice::ReadOnly);
    QByteArray fileData = fi.readAll();
    fi.close();
    dest += "/" + fileInfo.fileName();
    QUrl url(dest);
    qDebug() << url;
    dest = url.path();
    qDebug() << dest;
    usb2snes->sendFile(dest, fileData);
    return false;
}

Qt::DropActions Usb2SnesFileModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

Qt::ItemFlags Usb2SnesFileModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    Qt::ItemFlags toret;
    toret = Qt::ItemIsDropEnabled | defaultFlags;
    if (index.isValid() && !fileInfos.at(index.row()).dir)
        toret |= Qt::ItemIsDragEnabled;
    return toret;
}
