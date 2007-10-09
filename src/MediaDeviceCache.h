/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_MEDIADEVICECACHE_H
#define AMAROK_MEDIADEVICECACHE_H

#include "amarok_export.h"

#include <QObject>
#include <QMap>
#include <QString>

namespace Solid {
    class Device;
}

class MediaDeviceCache : public QObject
{
    Q_OBJECT

    public:

        enum DeviceType { SolidType, ManualType, InvalidType };

        static MediaDeviceCache* s_instance;
        static MediaDeviceCache* instance();
        
        /**
        * Creates a new MediaDeviceCache.
        * 
        */
        MediaDeviceCache();
        ~MediaDeviceCache();

        void refreshCache();
        MediaDeviceCache::DeviceType deviceType( const QString &udi );
        QStringList getAll() { return m_type.keys(); }

    signals:
        void addDevice( const QString &udi );
        void removeDevice( const QString &udi );

    public slots:
        void deviceAdded( const QString &udi );
        void deviceRemoved( const QString &udi );

    public:
        QMap<QString, MediaDeviceCache::DeviceType> m_type;
};

#endif /* AMAROK_MEDIADEVICECACHE_H */

