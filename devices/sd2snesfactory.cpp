#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log_sd2snesfact, "SD2SNESFactory")
#define sDebug() qCDebug(log_sd2snesfact())


#include <QSerialPortInfo>

#include "sd2snesfactory.h"



SD2SnesFactory::SD2SnesFactory()
{

}


QStringList SD2SnesFactory::listDevices()
{
    QStringList toret;
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos)
    {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber() << "Busy : " << usbinfo.isBusy();
        if (usbinfo.serialNumber() == "DEMO00000000")
                toret << "SD2SNES " + usbinfo.portName();
    }
    return toret;
}

ADevice *SD2SnesFactory::attach(QString deviceName)
{
    if (deviceName.left(3) != "COM" && deviceName.left(7) != "SD2SNES")
        return nullptr;

    // This is for apps that don't ask the device list
    if (deviceName.left(3) == "COM")
        deviceName = "SD2SNES " + deviceName;

    if (mapPortDev.contains(deviceName))
        return mapPortDev[deviceName];
    QString portName = deviceName.mid(8);
    SD2SnesDevice*  newDev = new SD2SnesDevice(portName);
    mapPortDev[deviceName] = newDev;
    m_devices.append(newDev);
    return newDev;
}

QString SD2SnesFactory::status()
{
    QList<QSerialPortInfo> sinfos = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo usbinfo, sinfos)
    {
        sDebug() << usbinfo.portName() << usbinfo.description() << usbinfo.serialNumber() << "Busy : " << usbinfo.isBusy();
        if (usbinfo.serialNumber() == "DEMO00000000")
                return QString(tr("SD2SNES detected on %1", "Arg is port name")).arg(usbinfo.portName());
    }
    return tr("No SD2Snes device detected.");
}

QString SD2SnesFactory::name() const
{
    return "SD2Snes";
}

bool SD2SnesFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Delete " << dev->name();
    mapPortDev.remove(dev->name());
    m_devices.removeAll(dev);
    dev->deleteLater();
    return true;
}
