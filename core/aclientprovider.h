 #pragma once

#include "aclient.h"
#include <QObject>

class AClientProvider : public QObject {
    Q_OBJECT
public:
    AClientProvider(QObject* parent = nullptr) : QObject(parent) {}
    QString name;
    virtual void    deleteClient(AClient* client) = 0;

signals:
    void newClient(AClient* client);
};
