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

bool SD2SnesFactory::deleteDevice(ADevice *dev)
{
    sDebug() << "Delete " << dev->name();
    mapPortDev.remove(dev->name());
    dev->deleteLater();
    return true;
}
