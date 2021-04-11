/*
 * Copyright (c) 2021 the QUsb2Snes authors.
 *
 * This file is part of QUsb2Snes.
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

#ifndef SNESCLASSICMEMORYFINDER_H
#define SNESCLASSICMEMORYFINDER_H

#include <QFile>
#include <QMainWindow>
#include <QTimer>
#include <stuffclient.h>

namespace Ui {
class SNESClassicMemoryFinder;
}

class SNESClassicMemoryFinder : public QMainWindow
{
    Q_OBJECT

public:
    explicit SNESClassicMemoryFinder(QWidget *parent = nullptr);
    ~SNESClassicMemoryFinder();

private slots:
    void on_scanButton_pressed();

private:
    Ui::SNESClassicMemoryFinder *ui;
    StuffClient*    stuffclient;
    QByteArray      canoePid;
    QTimer          checkTimer;
    QByteArray      fsfSongData;
    QByteArray      sramData;

    void            checkStuff();
};

#endif // SNESCLASSICMEMORYFINDER_H
