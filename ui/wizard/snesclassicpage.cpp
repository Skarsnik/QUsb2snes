#include "snesclassicpage.h"
#include "ui_snesclassicpage.h"

SNESClassicPage::SNESClassicPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::SNESClassicPage)
{
    ui->setupUi(this);
    socket = new QTcpSocket(this);
    checkstate = CHECK_SERVERSTUFF;
    done = false;
    timer = new QTimer(this);
    timer->setInterval(500);
}

void SNESClassicPage::initializePage()
{
    connect(socket, &QTcpSocket::connected, this, [=]
    {
        qDebug() << "Connected";
        timer->stop();
        if (checkstate == CHECK_SERVERSTUFF)
        {
            socket->write("CMD canoe-shvc --version\n");
        }
        if (checkstate == CHECK_TELNET)
        {
            ui->statusLabel->setText(tr("SNES Classic found, checking for serverstuff mod"));
            socket->close();
            checkstate = CHECK_SERVERSTUFF;
            socket->connectToHost("169.254.13.37", 1042);
        }
        timer->start();
    });
    connect(socket, &QTcpSocket::readyRead, this, [=]
    {
       sockDatas.append(socket->readAll());
       if (sockDatas.right(4) == QByteArray(4, 0))
       {
           sockDatas.chop(4);
           if (sockDatas == "fc349ac43140141277d2d6f964c1ee361fcd20ca\n")
           {
               ui->statusLabel->setText(tr("SNES Classic ready"));
               done = true;
               emit completeChanged();
           } else {
               ui->statusLabel->setText(tr("Your SNES Classic does not have the right CANOE-SHVC version, please update your kernel"));
           }
           sockDatas.clear();
       }
    });
    timer->start();
    socket->connectToHost("169.254.13.37", 1042);
    ui->statusLabel->setText(tr("Trying to connect to the SNES Classic."));
    connect(timer, &QTimer::timeout, this, [=]
    {
        qDebug() << socket->state();
        if (socket->state() != QTcpSocket::ConnectedState)
        {
            if (checkstate == CHECK_SERVERSTUFF)
            {
                socket->close();
                checkstate = CHECK_TELNET;
                socket->connectToHost("169.254.13.37", 23);
                return ;
            }
            if (checkstate == CHECK_TELNET)
            {
                socket->close();
                checkstate = CHECK_SERVERSTUFF;
                socket->connectToHost("169.254.13.37", 1042);
            }
        }
    });
}

SNESClassicPage::~SNESClassicPage()
{
    socket->close();
    delete ui;
}

bool SNESClassicPage::isComplete() const
{
    return done;
}

int SNESClassicPage::nextId() const
{
    return 42;
}

