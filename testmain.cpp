#include <QDebug>

#include "client/usb2snes.h"


Usb2Snes*   usb;


void    runThing()
{
    qDebug() << "Runnning thing";
    usb->getAddress(0xF50500, 2000);

    qApp->exit(0);
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
