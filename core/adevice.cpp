/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "adevice.h"

#include <QMetaEnum>

ADevice::ADevice(QObject *parent) : QObject(parent)
{
    m_attachError = "This device does not provide attach errors";
}

bool ADevice::hasVariaditeCommands()
{
    return false;
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
