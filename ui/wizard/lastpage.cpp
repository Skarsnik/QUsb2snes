#include "lastpage.h"
#include "ui_lastpage.h"

#include <QTimer>

LastPage::LastPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::LastPage)
{
    ui->setupUi(this);
    canNext = false;
    QTimer::singleShot(3000, this, [=]
    {
        canNext = true;
        emit completeChanged();
    });
}

LastPage::~LastPage()
{
    delete ui;
}

bool LastPage::isComplete() const
{
    return canNext;
}
