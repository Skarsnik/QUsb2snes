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

#ifndef USB2SNESFILEMODEL_H
#define USB2SNESFILEMODEL_H

#include <QAbstractListModel>
#include "usb2snes.h"

class Usb2SnesFileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit Usb2SnesFileModel(Usb2Snes *usb, QObject *parent = nullptr);

    // Basic functionality:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void    setPath(QString path);
    QString currentDir() const;
    bool    isDir(const QModelIndex &index) const;
    void    setDirOnly(bool);

private:
    Usb2Snes*   usb2snes;
    QString     m_currentDir;
    QList<Usb2Snes::FileInfo>   fileInfos;
    bool        dirOnly;

    // QAbstractItemModel interface

    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // USB2SNESFILEMODEL_H
