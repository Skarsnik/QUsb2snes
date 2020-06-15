#include "snesclassicmemoryfinder.h"
#include "ui_snesclassicmemoryfinder.h"

SNESClassicMemoryFinder::SNESClassicMemoryFinder(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SNESClassicMemoryFinder)
{
    ui->setupUi(this);
}

SNESClassicMemoryFinder::~SNESClassicMemoryFinder()
{
    delete ui;
}
