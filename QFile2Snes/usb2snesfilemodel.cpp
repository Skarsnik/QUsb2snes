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

#include "usb2snesfilemodel.h"

#include <QFileIconProvider>
#include <QDebug>
#include <QFile>


static bool sort_file_infos(Usb2Snes::FileInfo a, Usb2Snes::FileInfo b) {
    if (a.dir && ! b.dir) {
        return true;
    } else if (b.dir && ! a.dir) {
        return false;
    } else {
        return a.name.toLower() < b.name.toLower();
    }
}



Usb2SnesFileModel::Usb2SnesFileModel(Usb2Snes *usb, QObject *parent)
    : QAbstractListModel(parent)
{
        usb2snes = usb;
        m_currentDir = "";
        dirOnly = false;
        connect(usb2snes, &Usb2Snes::lsDone, this, [=] (QList<Usb2Snes::FileInfo> li) {
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
            std::sort(std::begin(fileInfos), std::end(fileInfos), sort_file_infos);
            emit endResetModel();
        });
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

/*
 * This sets the context to the new path, and updates the list of files
 * shown by reading from the USB2SNES
 */
void Usb2SnesFileModel::setPath(QString path)
{
    m_currentDir = path;
    usb2snes->ls(path);
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
