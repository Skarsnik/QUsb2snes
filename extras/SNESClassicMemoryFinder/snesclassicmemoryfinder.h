#ifndef SNESCLASSICMEMORYFINDER_H
#define SNESCLASSICMEMORYFINDER_H

#include <QMainWindow>

namespace Ui {
class SNESClassicMemoryFinder;
}

class SNESClassicMemoryFinder : public QMainWindow
{
    Q_OBJECT

public:
    explicit SNESClassicMemoryFinder(QWidget *parent = nullptr);
    ~SNESClassicMemoryFinder();

private:
    Ui::SNESClassicMemoryFinder *ui;
};

#endif // SNESCLASSICMEMORYFINDER_H
