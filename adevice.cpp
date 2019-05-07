#include "adevice.h"

#include <QMetaEnum>

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

QString ADevice::getFlagString(USB2SnesWS::extra_info_flags flag)
{
    static QMetaEnum me = USB2SnesWS::staticMetaObject.enumerator(USB2SnesWS::staticMetaObject.indexOfEnumerator("extra_info_flags"));
    return me.valueToKey(flag);
}
