#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebEngineView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QWebEngineView* view = new QWebEngineView(this);
    view->load(QUrl("http://www.multitroid.com"));
    view->setZoomFactor(0.8);
    setCentralWidget(view);
}

MainWindow::~MainWindow()
{
    delete ui;
}
