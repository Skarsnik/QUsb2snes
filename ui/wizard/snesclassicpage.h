#ifndef SNESCLASSICPAGE_H
#define SNESCLASSICPAGE_H

#include <QWizardPage>
#include <QTcpSocket>
#include <QTimer>

namespace Ui {
class SNESClassicPage;
}

class SNESClassicPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SNESClassicPage(QWidget *parent = nullptr);
    ~SNESClassicPage();
    bool    isComplete() const;
    int     nextId() const;
    void    initializePage();

private:
    enum CheckState
    {
        CHECK_TELNET,
        CHECK_SERVERSTUFF
    };
    Ui::SNESClassicPage *ui;
    QTcpSocket*     socket;
    CheckState      checkstate;
    QByteArray      sockDatas;
    bool            done;
    QTimer*         timer;
};

#endif // SNESCLASSICPAGE_H
