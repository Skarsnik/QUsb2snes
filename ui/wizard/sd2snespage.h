#ifndef SD2SNESPAGE_H
#define SD2SNESPAGE_H

#include <QWizardPage>
#include <devices/sd2snesdevice.h>

namespace Ui {
class Sd2SnesPage;
}

class Sd2SnesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit Sd2SnesPage(QWidget *parent = nullptr);
    ~Sd2SnesPage();
    void    initializePage();
    int     nextId() const;
    bool    isComplete() const;

private slots:
    void on_refreshButton_clicked();

private:
    Ui::Sd2SnesPage *ui;

    SD2SnesDevice*      sd2snesDevice;
    QString             comStatus;
    QString             sd2snesInfos;
    bool                deviceFound;
    bool                sd2snesInfoDone;
    void    refreshCOMPort();
    void    sd2snesGetInfos();

    // QWizardPage interface
public:

};

#endif // SD2SNESPAGE_H
