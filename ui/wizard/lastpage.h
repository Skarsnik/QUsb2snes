#ifndef LASTPAGE_H
#define LASTPAGE_H

#include <QWizardPage>

namespace Ui {
class LastPage;
}

class LastPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit LastPage(QWidget *parent = nullptr);
    ~LastPage();
    bool    isComplete() const;

private:
    Ui::LastPage *ui;
    bool        canNext;
};

#endif // LASTPAGE_H
