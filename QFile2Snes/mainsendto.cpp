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

#include "sendtodialog.h"

#include <QApplication>

static QTextStream cout(stdout);

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    /*if (QString(context.category) == "LowLevelTelnet")
        return ;*/
    //cout << msg;
    QString logString = QString("%6 %5 - %7: %1 \t(%2:%3, %4)").arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function).arg(context.category, 20).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    cout << logString << "\n";
    cout.flush();
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SendToDialog diag(a.arguments().at(1));

    QTranslator translator;
    QString locale = QLocale::system().name().split('_').first();
    translator.load(qApp->applicationDirPath() + "/i18n/sendtodialog_" + locale + ".qm");
    qApp->installTranslator(&translator);
    //SendToDialog diag("F:/Emulation/Super Mario World/Super Mario World.smc");
    //qInstallMessageHandler(myMessageOutput);
    if (!diag.init())
        exit(1);
    diag.show();
    return a.exec();
}
