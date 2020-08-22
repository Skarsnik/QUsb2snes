#include "snesclassicmemoryfinder.h"
#include "ui_snesclassicmemoryfinder.h"


SNESClassicMemoryFinder::SNESClassicMemoryFinder(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SNESClassicMemoryFinder)
{
    ui->setupUi(this);
    stuffclient = new StuffClient(this);
    connect(stuffclient, &StuffClient::connected, [=] {
        ui->statusLabel->setText("Connected");
    });
    connect(&checkTimer, &QTimer::timeout, this, &SNESClassicMemoryFinder::checkStuff);
    checkTimer.setInterval(500);
    checkTimer.start();
}

SNESClassicMemoryFinder::~SNESClassicMemoryFinder()
{
    delete ui;
}

const QString TESTROM_ID =  "CLV-U-BHYOP";

void SNESClassicMemoryFinder::checkStuff()
{
    ui->scanButton->setEnabled(false);
    /*ui->scanButton->setEnabled(false);
    if (!QFile::exists(qApp->applicationDirPath() + "/SNESClassic/free-software-song.ogg"))
    {
        ui->statusLabel->setText(tr("Missing the FSF Song file"));
        return ;
    } else {
        if (fsfSongData.isEmpty())
        {
            QFile fi(qApp->applicationDirPath() + "/SNESClassic/free-software-song.ogg");
            fi.open(QIODevice::ReadOnly);
            fsfSongData = fi.readAll();
            for (int i = 0; i < 1000; i++) {
                sramData[i] = fsfSongData.at(i) ^ 42;
            }
        }
    }*/

    if (!stuffclient->isConnected())
    {
        stuffclient->connect();
        return ;
    }
    //if(canoePid.isEmpty())
    {
    canoePid = stuffclient->waitForCommand("pidof canoe-shvc").trimmed();
    if (canoePid.isEmpty())
    {
       ui->statusLabel->setText("Canoe not running");
       return ;
    }
    }
    QString canoeArg = stuffclient->waitForCommand("ps | grep canoe-shvc | grep -v grep");
    /*if (canoeArg.indexOf(TESTROM_ID) == -1)
    {
        ui->statusLabel->setText("Not running the test rom");
        return ;
    }*/
    ui->scanButton->setEnabled(true);
}

const QByteArray wramData = QByteArray(40, 0) + QByteArray(40, 1) + QByteArray(40, 2);


QMap<uint32_t, QByteArray> SNESClassicMemoryFinder::getMemory(QByteArray cmd)
{
    QByteArray pmap = stuffclient->waitForCommand(cmd);
    QList<QByteArray> memEntries = pmap.split('\n');
    ui->logTextEdit->appendPlainText(QString("Found %1 memory entries\n").arg(memEntries.size()));

    QMap<uint32_t , QByteArray> out;

    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        bool ok;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);
        //qDebug() << ls.at(0);
        uint32_t memoryToTest = ls.at(0).toULong(&ok, 16);
        unsigned int size = ls.at(1).toUInt() * 1024;

        QByteArray memory = stuffclient->getMemory(canoePid.toUInt(), memoryToTest, size);

        out[memoryToTest] = memory;
    }
    return out;
}

uint32_t SNESClassicMemoryFinder::findPointer(uint32_t pointer, const QMap<uint32_t , QByteArray>& memories)
{
    foreach (uint32_t baseAddress, memories.keys())
    {
         const QByteArray& memory = memories[baseAddress];
         //unsigned long memoryToTest = ls.at(0).toULong(&ok, 16);
         unsigned int size = memory.size();

         //QByteArray memory = stuffclient->getMemory(canoePid.toUInt(), memoryToTest, size);

        //assert(memory.size());
         if(!memory.size())
         {
             continue;
         }
         //unsigned char arr[] = {0xE5, 0xF8, 0x0E};
         QByteArray data((char*)&pointer, 4);
         while(memory.lastIndexOf(data) != -1)
         {
             int dataLocation = memory.indexOf(data);
             if (dataLocation != -1 && dataLocation % 4 == 0)
             {
                 ui->logTextEdit->appendPlainText(QString("Find pointer at %1 , offset %2\n").arg(baseAddress, 0, 16).arg(dataLocation, 0, 16));
                 qDebug() << "Find pointer at " /*<< QString::number(memoryToTest, 16)*/ << QString::number(dataLocation, 16);
                 break;
                 //return baseAddress + dataLocation;
             }
         }
    }
    return 0;
}
#include <QtEndian>

void SNESClassicMemoryFinder::on_scanButton_pressed()
{
    checkTimer.stop();
    //QByteArray pmap = stuffclient->waitForCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    QByteArray pmap = stuffclient->waitForCommand(QByteArray("pmap ") + canoePid + " -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon");
    QList<QByteArray> memEntries = pmap.split('\n');
    ui->logTextEdit->appendPlainText(QString("Found %1 memory entries\n").arg(memEntries.size()));

    uint32_t addr = 0;

    QByteArray memEntryTest;
    qDebug() << "App path : " << qApp->applicationDirPath();

    foreach (QByteArray memEntry, memEntries)
    {
        if (memEntry.isEmpty())
            continue;
        QString s = memEntry;
        bool ok;
        QStringList ls = s.split(" ", QString::SkipEmptyParts);
        //qDebug() << ls.at(0);
        uint32_t memoryToTest = ls.at(0).toULong(&ok, 16);
        unsigned int size = ls.at(1).toUInt() * 1024;


        QByteArray memory = stuffclient->getMemory(canoePid.toUInt(), memoryToTest, size);

        QString filename = "D:\\scratch\\" + QString(memoryToTest) + ".bin";
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(memory);
        }
        else
        {
            qDebug() << file.errorString();

//            assert(false);

            // sad
        }

        unsigned char arr[] = {0xE5, 0xF8, 0x0E, 0x00, 0x84, 0x02, 0x00, 0x00, 0x00, 0xD5, 0x02, 0xCF, 0x04, 0x0F, 0x00, 0xFF, 0x1B, 0x02, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x02};
        QByteArray data((char*)arr, 8);
        //unsigned char arr[] = {0x70, 0x54, 0x11, 0x15};
        //QByteArray data((char*)arr, 4);
        //const int OFFSET = 0x02FB;
        const int RAM_OFFSET = 0x55BF4;
        int dataLocation = memory.indexOf(data);
        if (dataLocation != -1)// && dataLocation - OFFSET - RAM_OFFSET) % 0x1000 == 0)
        {
            ui->logTextEdit->appendPlainText(QString("Find addr at %1 , offset %2").arg(memoryToTest, 0, 16).arg(dataLocation, 0, 16));
            //ui->logTextEdit->appendPlainText(QByteArray(&memory.data()[dataLocation], 16+8).toHex());
            qDebug() << "Find addr at " << QString::number(memoryToTest, 16) << QString::number(dataLocation, 16);
            addr = memoryToTest + dataLocation - 0x20BEC;// - 0x1bf4;
            memEntryTest = memEntry;
            break;
        }
        memory.chop(memory.size() - dataLocation);
    }


    QMap<unsigned int, QByteArray> memories = getMemory((QByteArray("pmap ") + canoePid + " -x -q | grep rwx"));

    while(addr)
    {
        ui->logTextEdit->appendPlainText("Look for ptr");

        //while(newAddr == 0)
        {
            addr = findPointer(addr, memories);
          //  addr -= 4;
         //   qDebug() << "no";
        }

    }

/*
    auto a = stuffclient->getMemory canoePid.toUInt(), 0x223670, 4);
    uint32_t ap = qFromLittleEndian<uint32_t>(a.data());
    ui->logTextEdit->appendPlainText(QString(ap));

    a = stuffclient->getMemory(canoePid.toUInt(), ap + 0x20BEC, 4);
    ap = qFromLittleEndian<uint32_t>(a.data());
    ui->logTextEdit->appendPlainText(QString(ap));
*/
ui->logTextEdit->appendPlainText("No more ptr");

    ui->progressBar->setValue(100);
}
