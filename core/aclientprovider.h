 #pragma once

#include <QObject>

class AClient;

class AClientProvider : public QObject {
    Q_OBJECT
public:
    AClientProvider(QObject* parent = nullptr) : QObject(parent) {}
    QString name;
    virtual void    deleteClient(AClient* client) = 0;

signals:
    void newClient(AClient* client);
};
