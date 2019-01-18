#ifndef DEVICEFACTORY_H
#define DEVICEFACTORY_H

#include <QObject>

class DeviceFactory : public QObject
{
    Q_OBJECT
public:
    explicit DeviceFactory(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DEVICEFACTORY_H