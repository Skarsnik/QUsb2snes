#include "adevice.h"

ADevice::ADevice(QObject *parent) : QObject(parent)
{
    m_attachError = "This device does not provide attach errors";
}

bool ADevice::deleteOnClose()
{
    return false;
}

ADevice::State ADevice::state() const
{
    return m_state;
}

QString ADevice::attachError() const
{
    return m_attachError;
}
