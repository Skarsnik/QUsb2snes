#include <QDebug>

#include "client/usb2snes.h"


USB2snes*   usb;


void    runThing()
{
    qDebug() << "Runnning thing";
    usb->getAddress(0xF50500, 2000);

    qApp->exit(0);
}

void    stateChanged()
{
    qDebug() << "State Changed";
    if (usb->state() == USB2snes::Ready)
        runThing();
}

int main(int ac, char* ag[])
{
    QCoreApplication app(ac, ag);

    usb = new USB2snes(true);
    usb->connect();
    QObject::connect(usb, &USB2snes::stateChanged, stateChanged);
    //usb->getAddress()
    app.exec();
}
