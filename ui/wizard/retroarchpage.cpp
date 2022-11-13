#include "retroarchpage.h"
#include "ui_retroarchpage.h"

RetroArchPage::RetroArchPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::RetroArchPage)
{
    ui->setupUi(this);
    raHost = nullptr;
    infoDone = false;
}

RetroArchPage::~RetroArchPage()
{
    delete ui;
}

void RetroArchPage::startCheck()
{
    if (raHost != nullptr)
        raHost->deleteLater();
    raHost = new RetroArchHost("Test Host", this);
    raHost->setHostAddress(QHostAddress("127.0.0.1"));
    raHost->getInfos();
    ui->refreshButton->setEnabled(false);
    connect(raHost, &RetroArchHost::infoDone, this, [=](quint64 id)
    {
        ui->raVersionLabel->setText(QString(tr("RetroArch version : %1")).arg(raHost->version().toString()));
        ui->raContentLabel->setText(QString(tr("Current game played : %1")).arg(raHost->gameTitle()));
        ui->refreshButton->setEnabled(true);
        infoDone = true;
        emit completeChanged();
    });
    connect(raHost, &RetroArchHost::infoFailed, this, [=](quint64 id)
    {
       if (!raHost->version().isNull())
           ui->raVersionLabel->setText(tr("RetroArch version : %1").arg(raHost->version().toString()));
       ui->raContentLabel->setText(QString(tr("Info error : %1\nPlease insert a SNES Game, if you still see this error try another core")).arg(raHost->lastInfoError()));
       ui->refreshButton->setEnabled(true);
       infoDone = false;
       emit completeChanged();
    });

}

void RetroArchPage::initializePage()
{
    startCheck();
}

bool RetroArchPage::isComplete() const
{
    return infoDone;
}

int RetroArchPage::nextId() const
{
    return 42;
}



void RetroArchPage::on_refreshButton_clicked()
{
    startCheck();
}

