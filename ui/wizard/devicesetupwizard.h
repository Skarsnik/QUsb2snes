#ifndef DEVICESETUPWIZARD_H
#define DEVICESETUPWIZARD_H

#include <QCloseEvent>
#include <QWizard>

namespace Ui {
class DeviceSetupWizard;
}

class DeviceSetupWizard : public QWizard
{
    Q_OBJECT

public:
    enum DeviceSelected {
        SD2SNES,
        RETROARCH,
        SNESCLASSIC,
        NWA
    };
    Q_ENUM(DeviceSelected);
    explicit DeviceSetupWizard(QWidget *parent = nullptr);
    ~DeviceSetupWizard();
    DeviceSelected  deviceSelected() const;
    bool            contextMenu() const;

private:
    Ui::DeviceSetupWizard *ui;

    bool    showQuitMessage();

protected:
    void closeEvent(QCloseEvent *event);

    // QDialog interface
public slots:
    void reject();
};

#endif // DEVICESETUPWIZARD_H
