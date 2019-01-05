#include "qfile2snesw.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile2SnesW w;
    w.show();

    return a.exec();
}
