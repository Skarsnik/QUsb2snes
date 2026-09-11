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
    : QAbstractTableModel(parent)
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
        connect(usb2snes, &Usb2Snes::extendedlsDone, this, [=] (QList<Usb2Snes::FileInfo> li) {
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
        if (index.column() == 0)
            return QVariant(fileInfos.at(index.row()).name);
        if (index.column() == 1)
        {
            return QLocale().toString(fileInfos.at(index.row()).createdTime.toLocalTime(),
                                      QLocale::ShortFormat);
        }
        if (index.column() == 2 && fileInfos.at(index.row()).dir == false)
            return QVariant(QLocale().formattedDataSize(fileInfos.at(index.row()).size));
    }
    case Qt::DecorationRole:
    {
        if (index.column() != 0)
            return {};
        QFileIconProvider provid;
        if (fileInfos.at(index.row()).dir)
            return provid.icon((QFileIconProvider::Folder));
        else
            return provid.icon((QFileIconProvider::File));
    }
    case Qt::UserRole:
    {
        if (index.column() == 0)
            return QVariant(fileInfos.at(index.row()).name);
        if (index.column() == 1)
        {
            return QLocale().toString(fileInfos.at(index.row()).createdTime.toLocalTime(),
                                      QLocale::ShortFormat);
        }
        if (index.column() == 2 && fileInfos.at(index.row()).dir == true)
            return QVariant(0);
        if (index.column() == 2 && fileInfos.at(index.row()).dir == false)
            return QVariant(fileInfos.at(index.row()).size);
    }
    }
    return {};
}

/*
 * This sets the context to the new path, and updates the list of files
 * shown by reading from the USB2SNES
 */
void Usb2SnesFileModel::setPath(QString path)
{
    qDebug() << "Set PATH : " << path;
    m_currentDir = path;
    if (extended)
        usb2snes->extendedls(path);
    else
        usb2snes->ls(path);
}

void Usb2SnesFileModel::setExtended(bool e)
{
    qDebug() << "Set EXTENDED : " << e;
    if (extended == false && e == true)
    {
        beginInsertColumns(QModelIndex(), 1, 2);
        extended = e;
        endInsertColumns();
    }
    if (extended == true && e == false)
    {
        beginRemoveColumns(QModelIndex(), 1, 2);
        extended = e;
        endRemoveColumns();
    }


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
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Horizontal)
    {
        if (section == 0)
            return QVariant(tr("Name"));
        if (section == 1)
            return QVariant(tr("Date modified"));
        if (section == 2)
            return QVariant(tr("Size"));
    }
    return {};
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
    m_currentFile = dest;
    usb2snes->sendFile(dest, fileData);
    return false;
}

Qt::DropActions Usb2SnesFileModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

int Usb2SnesFileModel::columnCount(const QModelIndex &parent) const
{
    if (extended)
        return 3;
    return 1;
}

Qt::ItemFlags Usb2SnesFileModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);
    Qt::ItemFlags toret;
    toret = Qt::ItemIsDropEnabled | defaultFlags;
    if (index.isValid() && !fileInfos.at(index.row()).dir)
        toret |= Qt::ItemIsDragEnabled;
    return toret;
}

QString Usb2SnesFileModel::currentFile() const
{
    return m_currentFile;
}
