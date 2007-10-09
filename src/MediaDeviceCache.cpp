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

#define DEBUG_PREFIX "MediaDeviceCache"

#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/portablemediaplayer.h>

#include <QList>

#include "debug.h"
#include "MediaDeviceCache.h"


MediaDeviceCache* MediaDeviceCache::s_instance = 0;

MediaDeviceCache*
MediaDeviceCache::instance()
{
    static MediaDeviceCache sh;
    return &sh;
}

MediaDeviceCache::MediaDeviceCache() : QObject()
                             , m_type()
{
    DEBUG_BLOCK
    s_instance = this;
}

MediaDeviceCache::~MediaDeviceCache()
{
}

void
MediaDeviceCache::refreshCache()
{
    DEBUG_BLOCK
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString & ) ),
             this, SLOT( deviceAdded( const QString & ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString & ) ),
             this, SLOT( deviceRemoved( const QString & ) ) );
    QList<Solid::Device> deviceList = Solid::Device::listFromType( Solid::DeviceInterface::PortableMediaPlayer );
    Solid::Device temp;
    foreach( Solid::Device device, deviceList )
    {
        debug() << "Found Solid::DeviceInterface::PortableMediaPlayer with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();
        m_type[device.udi()] = MediaDeviceCache::SolidType;
    }
    QMap<QString, QString> manualDevices = KGlobal::config()->entryMap( "PortableDevices" );
    foreach( QString udi, manualDevices.keys() )
    {
        if( udi.startsWith( "manual" ) )
            m_type[udi] = MediaDeviceCache::ManualType;
    }
    foreach( QString udi, m_type.keys() )
        emit addDevice( udi );
}

void
MediaDeviceCache::deviceAdded( const QString &udi )
{
    DEBUG_BLOCK
    if( m_type.contains( udi ) )
    {
        debug() << "Duplicate UDI trying to be added: " << udi;
        return;
    }
    emit addDevice( udi );
}

void
MediaDeviceCache::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    if( m_type.contains( udi ) )
    {
        emit removeDevice( udi );
        return;
    }
    debug() << "Odd, got a deviceRemoved at udi " << udi << " but it didn't seem to exist in the first place...";
}

MediaDeviceCache::DeviceType
MediaDeviceCache::deviceType( const QString &udi )
{
    DEBUG_BLOCK
    if( m_type.contains( udi ) )
    {
        return m_type[udi];
    }
    return MediaDeviceCache::InvalidType;
}

#include "MediaDeviceCache.moc"

