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

#include <KConfig>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/portablemediaplayer.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>
#include <solid/storagevolume.h>

#include <QList>

#include "amarok.h"
#include "debug.h"
#include "MediaDeviceCache.h"


MediaDeviceCache* MediaDeviceCache::s_instance = 0;

MediaDeviceCache::MediaDeviceCache() : QObject()
                             , m_type()
                             , m_name()
                             , m_volumes()
{
    DEBUG_BLOCK
    s_instance = this;
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString & ) ),
             this, SLOT( slotAddSolidDevice( const QString & ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString & ) ),
             this, SLOT( slotRemoveSolidDevice( const QString & ) ) );
}

MediaDeviceCache::~MediaDeviceCache()
{
    s_instance = 0;
}

void
MediaDeviceCache::refreshCache()
{
    DEBUG_BLOCK
    m_type.clear();
    m_name.clear();
    QList<Solid::Device> deviceList = Solid::Device::listFromType( Solid::DeviceInterface::PortableMediaPlayer );
    Solid::Device temp;
    foreach( Solid::Device device, deviceList )
    {
        if( device.as<Solid::StorageDrive>() )
        {
            debug() << "Found Solid PMP that is also a StorageDrive, skipping";
            continue;
        }
        debug() << "Found Solid::DeviceInterface::PortableMediaPlayer with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();
        m_type[device.udi()] = MediaDeviceCache::SolidPMPType;
        m_name[device.udi()] = device.vendor() + " - " + device.product();
    }
    deviceList = Solid::Device::listFromType( Solid::DeviceInterface::StorageAccess );
    foreach( Solid::Device device, deviceList )
    {
        debug() << "Found Solid::DeviceInterface::StorageAccess with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();
        Solid::StorageAccess* ssa = device.as<Solid::StorageAccess>();
        if( ssa )
        {
            if( !m_volumes.contains( device.udi() ) )
            {
                connect( ssa, SIGNAL( accessibilityChanged(bool, const QString&) ),
                    this, SLOT( slotAccessibilityChanged(bool, const QString&) ) );
                m_volumes.append( device.udi() );
            }
            if( ssa->isAccessible() )
            {
                m_type[device.udi()] = MediaDeviceCache::SolidVolumeType;
                m_name[device.udi()] = ssa->filePath();
            }
            else
            {
                debug() << "Solid device is not accessible, will wait until it is to consider it added.";
            }
        }
    }
    KConfigGroup config = Amarok::config( "PortableDevices" );
    QMap<QString, QString> manualDevices = config.entryMap();
    foreach( const QString &udi, manualDevices.keys() )
    {
        if( udi.startsWith( "manual" ) )
        {
            debug() << "Found manual device with udi = " << udi;
            m_type[udi] = MediaDeviceCache::ManualType;
            m_name[udi] = udi.split( '|' )[1];
        }
    }
}

void
MediaDeviceCache::slotAddSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    Solid::Device device( udi );
    debug() << "Found new Solid device with udi = " << device.udi();
    debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();
    Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
    if( m_type.contains( udi ) )
    {
        debug() << "Duplicate UDI trying to be added: " << udi;
        return;
    }
    if( device.as<Solid::StorageDrive>() )
    {
        debug() << "Storage drive found, will wait for the volume";
        return;
    }
    else if( ssa )
    {
        debug() << "volume is generic storage";
        if( !m_volumes.contains( device.udi() ) )
        {
            connect( ssa, SIGNAL( accessibilityChanged(bool, const QString&) ),
                this, SLOT( slotAccessibilityChanged(bool, const QString&) ) );
            m_volumes.append( device.udi() );
        }
        if( ssa->isAccessible() )
        {
            m_type[udi] = MediaDeviceCache::SolidVolumeType;
            m_name[udi] = ssa->filePath();
        }
        else
        {
            debug() << "storage volume is not accessible right now, not adding.";
            return;
        }
    }
    else if( device.as<Solid::PortableMediaPlayer>() )
    {
        debug() << "device is a PMP";
        m_type[udi] = MediaDeviceCache::SolidPMPType;
        m_name[udi] = device.vendor() + " - " + device.product();
    }
    else
    {
        debug() << "udi " << udi << " does not describe a portable media player or storage volume";
        return;
    }
    emit deviceAdded( udi );
}

void
MediaDeviceCache::slotRemoveSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    debug() << "udi is: " << udi;
    Solid::Device device( udi );
    if( m_volumes.contains( udi ) )
    {
        disconnect( device.as<Solid::StorageAccess>(), SIGNAL( accessibilityChanged(bool, const QString&) ),
                    this, SLOT( slotAccessibilityChanged(bool, const QString&) ) );
        m_volumes.removeAll( udi );
    }
    if( m_type.contains( udi ) )
    {
        m_type.remove( udi );
        m_name.remove( udi );
        emit deviceRemoved( udi );
        return;
    }
    debug() << "Odd, got a deviceRemoved at udi " << udi << " but it didn't seem to exist in the first place...";
}

void
MediaDeviceCache::slotAccessibilityChanged( bool accessible, const QString &udi )
{
    DEBUG_BLOCK
    debug() << "accessibility of device " << udi << " has changed to accessible = " << (accessible ? "true":"false");
    if( accessible )
    {
        Solid::Device device( udi );
        m_type[udi] = MediaDeviceCache::SolidVolumeType;
        Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
        if( ssa )
            m_name[udi] = ssa->filePath();
        emit deviceAdded( udi );
        return;
    }
    else
    {
        if( m_type.contains( udi ) )
        {
            m_type.remove( udi );
            m_name.remove( udi );
            emit deviceRemoved( udi );
            return;
        }
        debug() << "Got accessibility changed to false but wasn't there in the first place...";
    }            
}

const MediaDeviceCache::DeviceType
MediaDeviceCache::deviceType( const QString &udi ) const
{
    DEBUG_BLOCK
    if( m_type.contains( udi ) )
    {
        return m_type[udi];
    }
    return MediaDeviceCache::InvalidType;
}

const QString
MediaDeviceCache::deviceName( const QString &udi ) const
{
    DEBUG_BLOCK
    if( m_name.contains( udi ) )
    {
        return m_name[udi];
    }
    return "ERR_NO_NAME"; //Should never happen!
}

bool
MediaDeviceCache::isGenericEnabled( const QString &udi ) const
{
    DEBUG_BLOCK
    if( m_type[udi] != MediaDeviceCache::SolidVolumeType )
    {
        debug() << "Not SolidVolumeType, returning false";
        return false;
    }
    Solid::Device device( udi );
    Solid::StorageAccess* ssa = device.as<Solid::StorageAccess>();
    if( !ssa || !ssa->isAccessible() )
    {
        debug() << "Not able to convert to StorageAccess or not accessible, returning false";
        return false;
    }
    if( device.parent().as<Solid::PortableMediaPlayer>() )
    {
        debug() << "Could convert parent to PortableMediaPlayer, returning true";
        return true;
    }
    KIO::UDSEntry udsentry;
    if( !KIO::NetAccess::stat( ssa->filePath() + "/.is_audio_player", udsentry, Amarok::mainWindow() ) )
    {
        debug() << "Did not stat .is_audio_player successfully, returning false";
        return false;
    }
    debug() << "Returning true";
    return true;
}

const QString
MediaDeviceCache::volumeMountPoint( const QString &udi ) const
{
    DEBUG_BLOCK
    Solid::Device device( udi );
    Solid::StorageAccess* ssa = device.as<Solid::StorageAccess>();
    if( !ssa || !ssa->isAccessible() )
    {
        debug() << "Not able to convert to StorageAccess or not accessible, returning empty";
        return QString();
    }
    return ssa->filePath();
}

#include "MediaDeviceCache.moc"

