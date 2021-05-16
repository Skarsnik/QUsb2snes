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

#include "snesclassicmemoryfinder.h"
#include "ui_snesclassicmemoryfinder.h"


SNESClassicMemoryFinder::SNESClassicMemoryFinder(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SNESClassicMemoryFinder)
{
    ui->setupUi(this);
    stuffclient = new StuffClient(this);
    connect(stuffclient, &StuffClient::connected, [=] {
        ui->statusLabel->setText("Connected");
    });
    connect(&checkTimer, &QTimer::timeout, this, &SNESClassicMemoryFinder::checkStuff);
    checkTimer.setInterval(500);
    checkTimer.start();
}

SNESClassicMemoryFinder::~SNESClassicMemoryFinder()
{
    delete ui;
}

const QString TESTROM_ID =  "CLV-U-BHYOP";

void SNESClassicMemoryFinder::checkStuff()
{
    ui->scanButton->setEnabled(false);
    if (!QFile::exists(qApp->applicationDirPath() + "/SNESClassic/free-software-song.ogg"))
    {
        ui->statusLabel->setText(tr("Missing the FSF Song file"));
        return ;
    } else {
        if (fsfSongData.isEmpty())
        {
            QFile fi(qApp->applicationDirPath() + "/SNESClassic/free-software-song.ogg");
            fi.open(QIODevice::ReadOnly);
            fsfSongData = fi.readAll();
            for (int i = 0; i < 1000; i++) {
                sramData[i] = fsfSongData.at(i) ^ 42;
            }
        }
    }
    if (!stuffclient->isConnected())
    {
        stuffclient->connect();
        return ;
    }
    canoePid = stuffclient->waitForCommand("pidof canoe-shvc").trimmed();
    if (canoePid.isEmpty())
    {
       ui->statusLabel->setText("Canoe not running");
       return ;
    }
    QString canoeArg = stuffclient->waitForCommand("ps | grep canoe-shvc | grep -v grep");
    /*if (canoeArg.indexOf(TESTROM_ID) == -1)
    {
        ui->statusLabel->setText("Not running the test rom");
        return ;
    }*/
    ui->scanButton->setEnabled(true);
}

const QByteArray wramData = QByteArray(40, 0) + QByteArray(40, 1) + QByteArray(40, 2);


void SNESClassicMemoryFinder::on_scanButton_pressed()
{
    checkTimer.stop();
    QByteArray pmap = stuffclient->waitForCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    QList<QByteArray> memEntries = pmap.split('\n');
    ui->logTextEdit->appendPlainText(QString("Found %1 memory entries\n").arg(memEntries.size()));
    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        bool ok;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);
        //qDebug() << ls.at(0);
        unsigned long memoryToTest = ls.at(0).toULong(&ok, 16);
        unsigned int size = ls.at(1).toUInt() * 1024;
        //qDebug() << "Testing " << memoryToTest << size;
        //if (!(ls.at(1) == "5092"  || ls.at(1) == "6444" || ls.at(1) == "8196"))
        //    continue;
        QByteArray memory = stuffclient->getMemory(canoePid.toUInt(), memoryToTest, size);
        //qDebug() << "Received memory : " << memory.size();

        int wramLocation = memory.indexOf(wramData);
        int romLocation = memory.indexOf(fsfSongData);
        int sramLocation = memory.indexOf(sramData);
        if (wramLocation != -1)
        {
            ui->logTextEdit->appendPlainText(QString("Find WRAM data at %1 , offset %2\n").arg(memoryToTest, 0, 16).arg(wramLocation - 50, 0, 16));
            qDebug() << "Find WRAM data at " << QString::number(memoryToTest, 16) << QString::number(wramLocation - 50, 16);
        }
        if (romLocation != -1)
        {
            ui->logTextEdit->appendPlainText(QString("Find ROM data at %1 , offset %2\n").arg(memoryToTest, 0, 16).arg(romLocation - 0x8000, 0, 16));
            qDebug() << "Find ROM data at " << QString::number(memoryToTest, 16) << QString::number(romLocation - 0x8000, 16);
        }
        if (sramLocation != -1)
        {
            ui->logTextEdit->appendPlainText(QString("Find SRAM data at %1 , offset %2\n").arg(memoryToTest, 0, 16).arg(sramLocation, 0, 16));
            qDebug() << "Find SRAM data at " << QString::number(memoryToTest, 16) << QString::number(sramLocation, 16);
        }
    }
    ui->progressBar->setValue(100);
}
