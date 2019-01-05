#include "usb2snesfilemodel.h"

Usb2SnesFileModel::Usb2SnesFileModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QVariant Usb2SnesFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}

QModelIndex Usb2SnesFileModel::index(int row, int column, const QModelIndex &parent) const
{
    // FIXME: Implement me!
}

QModelIndex Usb2SnesFileModel::parent(const QModelIndex &index) const
{
    // FIXME: Implement me!
}

int Usb2SnesFileModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

int Usb2SnesFileModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

QVariant Usb2SnesFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}
