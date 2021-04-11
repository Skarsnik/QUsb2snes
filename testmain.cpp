/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

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
