#ifndef DIRCHANGEDIALOG_H
#define DIRCHANGEDIALOG_H

#include <QDialog>

namespace Ui {
class DirChangeDialog;
}

class DirChangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DirChangeDialog(QWidget *parent = nullptr);
    ~DirChangeDialog();

private:
    Ui::DirChangeDialog *ui;
};

#endif // DIRCHANGEDIALOG_H
