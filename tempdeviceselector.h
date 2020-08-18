#ifndef TEMPDEVICESELECTOR_H
#define TEMPDEVICESELECTOR_H

#include <QDialog>

namespace Ui {
class TempDeviceSelector;
}

class TempDeviceSelector : public QDialog
{
    Q_OBJECT

public:
    explicit TempDeviceSelector(QWidget *parent = nullptr);
    ~TempDeviceSelector();

    QStringList devices;

private slots:
    void on_buttonBox_accepted();

private:
    Ui::TempDeviceSelector *ui;
};

#endif // TEMPDEVICESELECTOR_H
