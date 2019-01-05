#ifndef USB2SNESFILEMODEL_H
#define USB2SNESFILEMODEL_H

#include <QAbstractListModel>
#include "usb2snes.h"

class Usb2SnesFileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit Usb2SnesFileModel(USB2snes *usb, QObject *parent = nullptr);

    // Basic functionality:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void    setPath(QString path);
    QString currentDir() const;
    bool    isDir(const QModelIndex &index) const;
    void    setDirOnly(bool);

private:
    USB2snes*   usb2snes;
    QString     m_currentDir;
    QList<USB2snes::FileInfo>   fileInfos;
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
