#ifndef SENDTODIALOG_H
#define SENDTODIALOG_H

#include <QDialog>

namespace Ui {
class SendToDialog;
}

class SendToDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendToDialog(QWidget *parent = nullptr);
    ~SendToDialog();

private:
    Ui::SendToDialog *ui;
};

#endif // SENDTODIALOG_H
