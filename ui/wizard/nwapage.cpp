#include "nwapage.h"
#include "ui_nwapage.h"
#include <QTimer>

NWAPage::NWAPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::NWAPage)
{
    ui->setupUi(this);
    nwaClient = new EmuNWAccessClient(this); 
    done = false;
    retryTimer.setInterval(1000);
}

void NWAPage::initializePage()
{
    connect(nwaClient, &EmuNWAccessClient::connected, [=] {
        nwaClient->cmdEmulatorInfo();
    });
    connect(nwaClient, &EmuNWAccessClient::readyRead, [=] {
        EmuNWAccessClient::Reply rep = nwaClient->readReply();
        if (!rep.isError)
        {
            auto map = rep.toMap();
            ui->statusLabel->setText(QString("Found : %1. Version : %2").arg(map["name"]).arg(map["version"]));
            done = true;
            emit completeChanged();
        } else {
            ui->statusLabel->setText(QString("Error : %1").arg(rep.error));
        }
    });
    nwaClient->connectToHost("localhost", 0xBEEF);
    connect(&retryTimer, &QTimer::timeout, this, [=] {
       if (!nwaClient->isConnected())
       {
           nwaClient->connectToHost("localhost", 0xBEEF);
       }
    });
    retryTimer.start();
}


NWAPage::~NWAPage()
{
    delete ui;
}

int NWAPage::nextId() const
{
    return 42;
}

bool NWAPage::isComplete() const
{
    return done;
}


void NWAPage::on_refreshPushButton_clicked()
{
    nwaClient->disconnectFromHost();
    nwaClient->connectToHost("localhost", 0xBEEF);
}

