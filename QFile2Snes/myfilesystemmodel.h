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

#ifndef MYFILESYSTEMMODEL_H
#define MYFILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QObject>
#include <usb2snesclient.h>

class MyFileSystemModel : public QFileSystemModel
{
    Q_OBJECT
public:
    MyFileSystemModel();

    // QAbstractItemModel interface
public:
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void          setUsb2Snes(Usb2Snes* usb);
    const QString&      getFilePath() const;

    Qt::DropActions supportedDropActions() const;

signals:
    void    aboutToOverwroteFile(QByteArray);

private slots:
    void     OnUsbFileData(QByteArray data);

private:
    Usb2Snes*    usb2snes;
    int          m_fileSize;
    int          m_sizeReceived;
    QByteArray   m_fileData;
    QString      m_filePath;
};

#endif // MYFILESYSTEMMODEL_H
