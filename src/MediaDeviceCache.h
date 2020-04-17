/****************************************************************************************
 * Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_MEDIADEVICECACHE_H
#define AMAROK_MEDIADEVICECACHE_H

#include "amarok_export.h"

#include <QObject>
#include <QMap>
#include <QStringList>


class AMAROK_EXPORT MediaDeviceCache : public QObject
{
    Q_OBJECT

    public:

        enum DeviceType { SolidPMPType, SolidVolumeType, ManualType, SolidAudioCdType, SolidGenericType, InvalidType };

        static MediaDeviceCache* instance() { return s_instance ? s_instance : new MediaDeviceCache(); }

        /**
        * Creates a new MediaDeviceCache.
        * 
        */
        MediaDeviceCache();
        ~MediaDeviceCache() override;

        void refreshCache();
        const QStringList getAll() const { return m_type.keys(); }
        MediaDeviceCache::DeviceType deviceType( const QString &udi ) const;
        const QString deviceName( const QString &udi ) const;
        const QString device( const QString & udi ) const;
        bool isGenericEnabled( const QString &udi ) const;
        const QString volumeMountPoint( const QString &udi ) const;

    Q_SIGNALS:
        void deviceAdded( const QString &udi );
        void deviceRemoved( const QString &udi );
        void accessibilityChanged( bool accessible, const QString &udi );

    public Q_SLOTS:
        void slotAddSolidDevice( const QString &udi );
        void slotRemoveSolidDevice( const QString &udi );
        void slotAccessibilityChanged( bool accessible, const QString &udi );

    private:
        QMap<QString, MediaDeviceCache::DeviceType> m_type;
        QMap<QString, QString> m_name;
        QMap<QString, bool> m_accessibility;
        QStringList m_volumes;
        static MediaDeviceCache* s_instance;
};

#endif /* AMAROK_MEDIADEVICECACHE_H */

