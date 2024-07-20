#ifndef SYSTRAYWIDGET_H
#define SYSTRAYWIDGET_H

#include <QWidget>
#include <QMenu>

namespace Ui {
class SysTrayWidget;
}

class SysTrayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysTrayWidget(QWidget *parent = nullptr);
    ~SysTrayWidget();
    void    initDeviceStatus();
    void    clearClientStatus();
    void    addDeviceStatus(QString status);
    void    noDevicesStatus();
    void    addClientStatus(QString status);
    QMenu   *contextMenu;

private slots:
    void on_pushButton_clicked();

private:
    Ui::SysTrayWidget *ui;
    bool        firstDeviceStatus;
    bool        firstClientStatus;
};

#endif // SYSTRAYWIDGET_H
