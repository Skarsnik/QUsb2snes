#ifndef SENDTODIALOG_H
#define SENDTODIALOG_H

#include <QDialog>
#include <QFileInfo>
#include "usb2snes.h"

namespace Ui {
class SendToDialog;
}

class SendToDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendToDialog(QString filePath, QWidget *parent = nullptr);
    bool init();
    ~SendToDialog();


private slots:
    void    onUsb2SnesStateChanged();
    void    onUsb2SnesDisconnected();
    void    onTimerTimeout();
    void    onRecoTimerTimeout();

    void on_pushButton_clicked();

private:
    Ui::SendToDialog *ui;
    QFileInfo   fileInfos;
    USB2snes*   usb2snes;
    QProcess    wsServer;
    QSettings*  settings;
    QTimer      progressTimer;
    QTimer      recoTimer;
    QString     countdownString;
    bool        sendingFile;
    int         progressStep;

    bool    checkForUsb2SnesServer();
    void    setStatusLabel(QString message);
    void    transfertFile();
    void    attachToSD2Snes();
};

#endif // SENDTODIALOG_H
