#include "devicefactory.h"

DeviceFactory::DeviceFactory(QObject *parent) : QObject(parent)
{

}

QList<ADevice *> DeviceFactory::getDevices()
{
    return m_devices;
}

ADevice *DeviceFactory::attach(QString deviceName)
{
    m_attachError = "";
    foreach(ADevice* dev, m_devices)
    {
        if (dev->name() == deviceName)
            return dev;
    }
    //m_attachError = "Device named " + deviceName + " Not found";
    return nullptr;
}

QString DeviceFactory::attachError() const
{
    return m_attachError;
}

bool DeviceFactory::hasAsyncListDevices()
{
    return false;
}

QStringList DeviceFactory::getDevicesName() const
{
    return m_listDeviceNames;
}
