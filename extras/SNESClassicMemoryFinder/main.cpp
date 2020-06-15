#include "snesclassicmemoryfinder.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SNESClassicMemoryFinder w;
    w.show();

    return a.exec();
}
