#include <QScreen>
#include "systraywidget.h"
#include "ui_systraywidget.h"
#include <QDebug>

SysTrayWidget::SysTrayWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SysTrayWidget)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowTitle("QUsb2Snes - v" + qApp->applicationVersion());
    firstDeviceStatus = true;
    firstClientStatus = true;
}

SysTrayWidget::~SysTrayWidget()
{
    delete ui;
}

void SysTrayWidget::initDeviceStatus()
{
    ui->devicesLabel->setText(tr("Getting the devices status"));
    firstDeviceStatus = true;
}

void SysTrayWidget::clearClientStatus()
{
    ui->applicationLabel->setText(tr("No application connected"));
}

void SysTrayWidget::addDeviceStatus(QString status)
{
    if (firstDeviceStatus == true)
    {
        ui->devicesLabel->setText("");
        firstDeviceStatus = false;
    }
    QString text = ui->devicesLabel->text();
    text.append(status + "\n");
    ui->devicesLabel->setText(text);
}

void SysTrayWidget::addClientStatus(QString status)
{
    if (firstClientStatus == true)
    {
        ui->applicationLabel->setText("");
        firstClientStatus = false;
    }
    QString text = ui->applicationLabel->text();
    text.append(status + "\n");
    ui->applicationLabel->setText(text);
}

void SysTrayWidget::on_pushButton_clicked()
{
    QPoint tray_center   = this->geometry().center();
    QRect  screen_rect   = qApp->screenAt(tray_center)->geometry();
    QPoint screen_center = screen_rect.center();

    Qt::Corner corner = Qt::TopLeftCorner;
    if (tray_center.x() > screen_center.x() && tray_center.y() <= screen_center.y())
        corner = Qt::TopRightCorner;
    else if (tray_center.x() > screen_center.x() && tray_center.y() > screen_center.y())
        corner = Qt::BottomRightCorner;
    else if (tray_center.x() <= screen_center.x() && tray_center.y() > screen_center.y())
        corner = Qt::BottomLeftCorner;
    QPoint p = mapToGlobal(ui->pushButton->geometry().topRight());
    if (corner == Qt::TopLeftCorner || corner == Qt::TopRightCorner)
    {
        p.setY(p.y() + ui->pushButton->geometry().height());
    } else {
        p.setY(p.y() - contextMenu->sizeHint().height());
    }
    if (corner == Qt::TopRightCorner)
        p.setX(p.x() - contextMenu->sizeHint().width());
    contextMenu->move(p);
    contextMenu->show();
}

