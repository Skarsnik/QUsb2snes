#include "diagnosticdialog.h"
#include "ui_diagnosticdialog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(log_diagnosticdialog, "DiagDialog")
#define sDebug() qCDebug(log_diagnosticdialog)
#define sInfo() qCInfo(log_diagnosticdialog)

static QStringList getJsonResults(QString json)
{
    QStringList toret;
    QJsonDocument   jdoc = QJsonDocument::fromJson(json.toLatin1());
    if (!jdoc.object()["Results"].toArray().isEmpty())
    {
        QJsonArray jarray = jdoc.object()["Results"].toArray();
        foreach(QVariant entry, jarray.toVariantList())
        {
            toret << entry.toString();
        }
    }
    return toret;
}

void DiagnosticDialog::sendRequest(QString opCode, QStringList operands)
{
    QJsonArray      jOp;
    QJsonObject     jObj;

    jObj["Opcode"] = opCode;
    if (!operands.isEmpty())
        jObj["Operands"] = jOp;
    testSocket.sendTextMessage(QJsonDocument(jObj).toJson());
}


DiagnosticDialog::DiagnosticDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DiagnosticDialog)
{
    ui->setupUi(this);
    QString qusbText = "QUsb2Snes " + qApp->applicationVersion();
#ifdef GIT_VERSION
    qusbText.append(" Git Version : " + GIT_VERSION);
#endif
    QString qtVersion = QString("Qt versions : Compiled on ") + QT_VERSION_STR + " Running on " + qVersion();
    QString os;
#ifdef Q_OS_WIN32
    os = "Windows 32 bits";
#endif
#ifdef Q_OS_WIN64
    os = "Windows 64 bits";
#endif
#ifdef Q_OS_MACOS
    os = "MacOs";
#endif
#ifdef Q_OS_LINUX
    os = "Linux";
#endif

    ui->qusbLabel->setText(qusbText + "\n" + qtVersion + "\nRunning on " + os);
    ui->groupBox->setLayout(new QVBoxLayout());
    connect(&testSocket, &QWebSocket::connected, this, [=] {
        sDebug() << "Socket Connected";
        sendRequest("DeviceList");
    });
    connect(&testSocket, &QWebSocket::textMessageReceived, this, [=](QString msg){
        sDebug() << "Message received : " << msg;
        switch(socketState)
        {
        case TestSocketState::DEVICELIST :
        {
            QStringList devices = getJsonResults(msg);
            if (devices.isEmpty())
            {
                ui->deviceWebsocketInfosLabel->setText("No device found");
                testSocket.close();
            } else {
                ui->deviceWebsocketInfosLabel->setText("Founds devices : " + devices.join("-"));
                socketState = TestSocketState::INFO;
                sendRequest("Attach", QStringList() << devices.first());
            }
            break;
        }
        case TestSocketState::INFO :
        {
            QStringList devices = getJsonResults(msg);
            ui->deviceInternalInfosLabel->setText("Type : " + devices.at(1) + " - Version : " + devices.at(0) + " - Rom : " + devices.at(2));
            break;
        }
        }
    });
}

void DiagnosticDialog::setWSServer(WSServer *serv)
{
    server = serv;
    connect(server, &WSServer::newDeviceFactoryStatus, this, [=](DeviceFactory::DeviceFactoryStatus status) {
        QString statusString;
        statusString = status.name + " : ";
        if (status.deviceNames.isEmpty())
        {
            if (status.generalError == Error::DeviceFactoryError::DFE_NO_ERROR)
            {
                statusString.append(QString("%1").arg(status.statusString()));
            } else {
                statusString.append(QString("%1").arg(status.errorString()));
            }
        } else {
            for(const QString& name : qAsConst(status.deviceNames))
            {
                statusString.append("\n       ");
                auto& deviceStatus = status.deviceStatus[name];
                if (deviceStatus.error == Error::DeviceError::DE_NO_ERROR)
                {
                    QStringList clients = server->getClientsName(name);
                    //TODO move the name in the full translation string
                    statusString.append(name + ": ");
                    if (clients.isEmpty())
                    {
                        statusString.append(tr("ready, no client connected"));
                    } else {
                        statusString.append(clients.join(", "));
                    }
                } else {
                    statusString.append(name + " : " + deviceStatus.errorString());
                }
                sDebug() << "name" << statusString;
            }
        }
        QString text = ui->deviceInternalInfosLabel->text();
        text.append(statusString + "\n");
        ui->deviceInternalInfosLabel->setText(text);
    });
    connect(server, &WSServer::deviceFactoryStatusDone, this, [=]
    {

    });
}



DiagnosticDialog::~DiagnosticDialog()
{
    delete ui;
}


int DiagnosticDialog::exec()
{
    sDebug() << "Started";
    ui->deviceInternalInfosLabel->setText("Checking device status");
    socketState = TestSocketState::DEVICELIST;
    for (const QString& devFactName : server->deviceFactoryNames())
    {
        sDebug() << ui->groupBox->layout();
        ui->groupBox->layout()->addWidget(new QLabel(devFactName));
    }
    sDebug() << "Opening websocket";
    server->requestDeviceStatus();
    ui->deviceInternalInfosLabel->setText("");
    testSocket.open(QUrl("ws://localhost:" + QString::number(USB2SnesWS::defaultPort)));
    return QDialog::exec();
}


void DiagnosticDialog::done(int i)
{
    testSocket.close();
    QDialog::done(i);
}
