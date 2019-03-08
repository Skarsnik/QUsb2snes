#ifndef MYFILESYSTEMMODEL_H
#define MYFILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QObject>
#include <usb2snes.h>

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
