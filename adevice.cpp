#include "adevice.h"

ADevice::ADevice(QObject *parent) : QObject(parent)
{

}

ADevice::State ADevice::state() const
{
    return m_state;
}
