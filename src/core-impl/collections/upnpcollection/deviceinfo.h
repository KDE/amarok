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

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QString>
#include <QDBusArgument>

class DeviceInfo;

class DeviceInfo0_1_0;
class DeviceInfo0_2_0;

extern const QDBusArgument& operator>>( const QDBusArgument& argument, DeviceInfo0_1_0& device );

typedef QMap<QString, QString> DeviceDetailsMap;

class DeviceInfo
{
public:
    const QString& type() const;
    const QString& friendlyName() const;
    const QString& manufacturerName() const;
    const QString& modelDescription() const;
    const QString& modelName() const;
    const QString& modelNumber() const;
    const QString& serialNumber() const;
    const QString& udn() const;
    QString uuid() const;
    const QString& presentationUrl() const;

    const QString& host() const;
    int port() const;

    const QString& parentDeviceUdn() const;

    bool isValid() const;

protected:
    QString m_type;
    QString m_friendlyName;
    QString m_manufacturerName;
    QString m_modelDescription;
    QString m_modelName;
    QString m_modelNumber;
    QString m_serialNumber;
    QString m_udn;
    QString m_presentationUrl;

    QString m_host;
    int m_port;

    QString m_parentDeviceUdn;

};

inline bool DeviceInfo::isValid() const
{
  return !m_type.isNull();
}

inline const QString& DeviceInfo::type() const
{
    return m_type;
}

inline const QString& DeviceInfo::friendlyName() const
{
    return m_friendlyName;
}

inline const QString& DeviceInfo::manufacturerName() const
{
    return m_manufacturerName;
}

inline const QString& DeviceInfo::modelDescription() const
{
    return m_modelDescription;
}

inline const QString& DeviceInfo::modelName() const
{
    return m_modelName;
}

inline const QString& DeviceInfo::modelNumber() const
{
    return m_modelNumber;
}

inline const QString& DeviceInfo::serialNumber() const
{
    return m_serialNumber;
}

inline const QString& DeviceInfo::udn() const
{
    return m_udn;
}

inline const QString& DeviceInfo::presentationUrl() const
{
    return m_presentationUrl;
}

inline const QString& DeviceInfo::host() const
{
    return m_host;
}

inline int DeviceInfo::port() const
{
    return m_port;
}

inline const QString& DeviceInfo::parentDeviceUdn() const
{
    return m_parentDeviceUdn;
}

inline QString DeviceInfo::uuid() const
{
  return QString( udn() ).replace( "uuid:", "" );
}

class DeviceInfo0_1_0 : public DeviceInfo
{
friend const QDBusArgument& operator>>( const QDBusArgument& argument, DeviceInfo0_1_0& device );
};

class DeviceInfo0_2_0 : public DeviceInfo
{
public:
    explicit DeviceInfo0_2_0( const DeviceDetailsMap &map )
    {
        m_type = map.value( "deviceType" );
        m_friendlyName = map.value( "friendlyName" );
        m_manufacturerName = map.value( "manufacturer" );
        m_modelDescription = map.value( "modelDescription" );
        m_modelName = map.value( "modelName" );
        m_modelNumber = map.value( "modelNumber" );
        m_serialNumber = map.value( "serialNumber" );
        m_udn = map.value( "UDN" );
        m_presentationUrl = map.value( "presentationURL" );

        m_host = map.value( "ipAddress" );
        m_port = map.value( "ipPortNumber" ).toUInt();

        m_parentDeviceUdn = map.value( "parentDeviceUDN" );

    }
};

Q_DECLARE_METATYPE( DeviceInfo )
Q_DECLARE_METATYPE( DeviceInfo0_1_0 )
Q_DECLARE_METATYPE( DeviceDetailsMap )
#endif
