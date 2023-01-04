#ifndef RETROARCHPAGE_H
#define RETROARCHPAGE_H

#include <QWizardPage>
#include <devices/retroarchhost.h>

namespace Ui {
class RetroArchPage;
}

class RetroArchPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit RetroArchPage(QWidget *parent = nullptr);
    ~RetroArchPage();

private:
    Ui::RetroArchPage *ui;

    RetroArchHost*  raHost;
    bool            infoDone;

    void startCheck();
public:
    void initializePage();
    bool isComplete() const;
    int nextId() const;
private slots:
    void on_refreshButton_clicked();
};

#endif // RETROARCHPAGE_H
