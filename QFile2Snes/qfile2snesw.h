#ifndef QFILE2SNESW_H
#define QFILE2SNESW_H

#include <QMainWindow>

namespace Ui {
class QFile2SnesW;
}

class QFile2SnesW : public QMainWindow
{
    Q_OBJECT

public:
    explicit QFile2SnesW(QWidget *parent = nullptr);
    ~QFile2SnesW();

private:
    Ui::QFile2SnesW *ui;
};

#endif // QFILE2SNESW_H
