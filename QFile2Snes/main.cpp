#include "qfile2snesw.h"
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
    qDebug() << a.arguments();
    if (a.arguments().size() == 2)
    {
        SendToDialog diag("F:/Tmp/SMALttP - sm-casual_alttp-no-glitches-v9.2_normal-open_morph-vanilla_ganon_combo_411066146.sfc");
        if (!diag.init())
            exit(1);
        diag.show();
        return a.exec();
    }
    //qInstallMessageHandler(myMessageOutput);
    QFile2SnesW w;
    w.show();
    return a.exec();
}
