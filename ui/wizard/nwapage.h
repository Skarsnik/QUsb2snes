#ifndef NWAPAGE_H
#define NWAPAGE_H

#include <QWidget>
#include <QWizardPage>
#include <QTimer>

#include <emunwaccessclient.h>

namespace Ui {
class NWAPage;
}

class NWAPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit NWAPage(QWidget *parent = nullptr);
    ~NWAPage();
    int nextId() const;
    bool isComplete() const;
    void    initializePage();


private slots:
    void on_refreshPushButton_clicked();

private:
    Ui::NWAPage *ui;
    EmuNWAccessClient* nwaClient;
    QTimer  retryTimer;
    bool    done;

};

#endif // NWAPAGE_H
