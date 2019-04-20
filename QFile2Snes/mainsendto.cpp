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
