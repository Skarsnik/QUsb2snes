#ifndef DEVICESELECTORPAGE_H
#define DEVICESELECTORPAGE_H

#include <QWizardPage>

namespace Ui {
class DeviceSelectorPage;
}

class DeviceSelectorPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit DeviceSelectorPage(QWidget *parent = nullptr);
    ~DeviceSelectorPage();

private:
    Ui::DeviceSelectorPage *ui;

    // QWizardPage interface
public:
    bool    isComplete() const;
    int     nextId() const;

};

#endif // DEVICESELECTORPAGE_H
