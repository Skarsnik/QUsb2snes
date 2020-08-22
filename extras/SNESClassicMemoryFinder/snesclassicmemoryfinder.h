#ifndef SNESCLASSICMEMORYFINDER_H
#define SNESCLASSICMEMORYFINDER_H

#include <QFile>
#include <QMainWindow>
#include <QTimer>
#include <stuffclient.h>

namespace Ui {
class SNESClassicMemoryFinder;
}

class SNESClassicMemoryFinder : public QMainWindow
{
    Q_OBJECT

public:
    explicit SNESClassicMemoryFinder(QWidget *parent = nullptr);
    ~SNESClassicMemoryFinder();
    QMap<uint32_t , QByteArray> getMemory(QByteArray cmd);
    uint32_t findPointer(uint32_t pointer, const QMap<uint32_t , QByteArray>& memories);


private slots:
    void on_scanButton_pressed();

private:
    Ui::SNESClassicMemoryFinder *ui;
    StuffClient*    stuffclient;
    QByteArray      canoePid;
    QTimer          checkTimer;
    QByteArray      fsfSongData;
    QByteArray      sramData;

    void            checkStuff();
};

#endif // SNESCLASSICMEMORYFINDER_H
