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
    usb2snes->getFile(sd2snesFilePath);
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

void MyFileSystemModel::setUsb2Snes(Usb2Snes *usb)
{
    usb2snes = usb;
    connect(usb2snes,  &Usb2Snes::getFileDataGet, this, &MyFileSystemModel::OnUsbFileData);
    connect(usb2snes, &Usb2Snes::getFileSizeGet, this, [=](unsigned int size) {
        m_fileSize = size;
    });
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
