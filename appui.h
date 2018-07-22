#ifndef APPUI_H
#define APPUI_H

#include <QObject>

class AppUi : public QObject
{
    Q_OBJECT
public:
    explicit AppUi(QObject *parent = nullptr);

signals:

public slots:
};

#endif // APPUI_H