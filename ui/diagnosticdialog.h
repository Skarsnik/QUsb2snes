#ifndef DIAGNOSTICDIALOG_H
#define DIAGNOSTICDIALOG_H

#include "core/wsserver.h"

#include <QDialog>

namespace Ui {
class DiagnosticDialog;
}

class DiagnosticDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiagnosticDialog(QWidget *parent = nullptr);
    void    setWSServer(WSServer* serv);
    ~DiagnosticDialog();

private:
    Ui::DiagnosticDialog *ui;

    // QDialog interface
public slots:
    int exec();
    void done(int);
private:
    enum TestSocketState {
        NONE,
        DEVICELIST,
        INFO
    };

    WSServer* server;
    QWebSocket testSocket;
    TestSocketState socketState;
    void sendRequest(QString opCode, QStringList operands = QStringList());
};

#endif // DIAGNOSTICDIALOG_H
