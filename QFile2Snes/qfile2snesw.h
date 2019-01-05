#ifndef QFILE2SNESW_H
#define QFILE2SNESW_H

#include <QMainWindow>
#include <QSettings>
#include "usb2snes.h"
#include "usb2snesfilemodel.h"

namespace Ui {
class QFile2SnesW;
}

class QFile2SnesW : public QMainWindow
{
    Q_OBJECT

public:
    enum State {
      NOTCONNECTED = 0,
      CONNECTING,
      IDLE,
      SENDINDFILE,
      GETTINGFILE,
    };
    explicit QFile2SnesW(QWidget *parent = nullptr);
    ~QFile2SnesW();

private slots:
    void on_driveComboBox_currentIndexChanged(int index);

    void on_localFilesListView_doubleClicked(const QModelIndex &index);

    void    onUsb2SnesStateChanged();

    void on_usb2snesListView_doubleClicked(const QModelIndex &index);

    void    onUsb2SnesFileSendProgress(int size);

    void    onSDViewSelectionChanged(const QItemSelection& selected, const QItemSelection &deselected);
    void    onSDViewCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

    void on_renameButton_clicked();

    void on_deleteButton_clicked();

private:
    State               m_state;
    Ui::QFile2SnesW     *ui;
    QSettings*          m_settings;
    USB2snes*           usb2snes;
    Usb2SnesFileModel*  usb2snesModel;

    void    setLFilepath(QString path);
    bool    listAndAttach();
};

#endif // QFILE2SNESW_H
