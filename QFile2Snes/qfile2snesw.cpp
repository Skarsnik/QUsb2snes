#include "qfile2snesw.h"
#include "ui_qfile2snesw.h"

QFile2SnesW::QFile2SnesW(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QFile2SnesW)
{
    ui->setupUi(this);
}

QFile2SnesW::~QFile2SnesW()
{
    delete ui;
}
