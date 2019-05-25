#include <QDebug>

#include "client/usb2snes.h"


Usb2Snes*   usb;


void    testSd2Snes()
{
    usb->ls("/");
    usb->getAddress(0xF50500, 2000);
    QByteArray data = usb->getAddress(0xF50500, 2);
    usb->setAddress(0xF50500, data);
    qApp->exit(0);
}

void    runThing()
{
    qDebug() << "Runnning thing";
    if (usb->infos().versionString == "SD2SNES")
    {
        testSd2Snes();
    }
}

void    stateChanged()
{
    qDebug() << "State Changed";
    if (usb->state() == Usb2Snes::Ready)
        runThing();
}

int main(int ac, char* ag[])
{
    QCoreApplication app(ac, ag);

    usb = new Usb2Snes(true);
    usb->connect();
    QObject::connect(usb, &Usb2Snes::stateChanged, stateChanged);
    //usb->getAddress()
    app.exec();
}
