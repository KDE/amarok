/*
    This file is part of the UPnP MediaServer Kioslave library, part of the KDE project.

    Copyright 2010 Nikhil Marathe <nsm.nikhil@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Adapted from Friedrich Kossebau's Cagibi daemon.
 */

#include "dbuscodec.h"

// Qt
#include <QtDBus/QDBusArgument>
#include <QtCore/QUrl>

QDBusArgument& operator<<( QDBusArgument& argument, const DeviceInfo& device )
{
    argument.beginStructure();

    argument << device.type()
             << device.friendlyName()
             << device.manufacturerName()
             << device.modelDescription()
             << device.modelName()
             << device.modelNumber()
             << device.serialNumber()
             << device.udn()
             << device.presentationUrl()
             << device.host()
             << device.port()
             << device.parentDeviceUdn();

    argument.endStructure();

    return argument;
}

const QDBusArgument& operator>>( const QDBusArgument& argument,
                                 DeviceInfo& device )
{
    argument.beginStructure();

    argument >> device.m_type
             >> device.m_friendlyName
             >> device.m_manufacturerName
             >> device.m_modelDescription
             >> device.m_modelName
             >> device.m_modelNumber
             >> device.m_serialNumber
             >> device.m_udn
             >> device.m_presentationUrl
             >> device.m_host
             >> device.m_port
             >> device.m_parentDeviceUdn;

    argument.endStructure();

    return argument;
}
