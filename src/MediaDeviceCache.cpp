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

#define DEBUG_PREFIX "MediaDeviceCache"

#include "MediaDeviceCache.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/PortableMediaPlayer>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include <QDir>
#include <QFile>
#include <QList>

#include <KConfigGroup>

MediaDeviceCache* MediaDeviceCache::s_instance = nullptr;

MediaDeviceCache::MediaDeviceCache() : QObject()
                             , m_type()
                             , m_name()
                             , m_volumes()
{
    DEBUG_BLOCK
    s_instance = this;
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
             this, &MediaDeviceCache::slotAddSolidDevice );
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
             this, &MediaDeviceCache::slotRemoveSolidDevice );
}

MediaDeviceCache::~MediaDeviceCache()
{
    s_instance = nullptr;
}

void
MediaDeviceCache::refreshCache()
{
    DEBUG_BLOCK
    m_type.clear();
    m_name.clear();
    QList<Solid::Device> deviceList = Solid::Device::listFromType( Solid::DeviceInterface::PortableMediaPlayer );
    for( const Solid::Device &device : deviceList )
    {
        if( device.as<Solid::StorageDrive>() )
        {
            debug() << "Found Solid PMP that is also a StorageDrive, skipping";
            continue;
        }
        debug() << "Found Solid::DeviceInterface::PortableMediaPlayer with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();
        m_type[device.udi()] = MediaDeviceCache::SolidPMPType;
        m_name[device.udi()] = device.vendor() + QStringLiteral(" - ") + device.product();
    }
    deviceList = Solid::Device::listFromType( Solid::DeviceInterface::StorageAccess );
    for( const Solid::Device &device : deviceList )
    {
        debug() << "Found Solid::DeviceInterface::StorageAccess with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();

        const Solid::StorageAccess* ssa = device.as<Solid::StorageAccess>();

        if( ssa )
        {
            if( !m_volumes.contains( device.udi() ) )
            {
                connect( ssa, &Solid::StorageAccess::accessibilityChanged,
                    this, &MediaDeviceCache::slotAccessibilityChanged );
                m_volumes.append( device.udi() );
            }
            if( ssa->isAccessible() )
            {
                m_type[device.udi()] = MediaDeviceCache::SolidVolumeType;
                m_name[device.udi()] = ssa->filePath();
                m_accessibility[ device.udi() ] = true;
            }
            else
            {
                m_accessibility[ device.udi() ] = false;
                debug() << "Solid device is not accessible, will wait until it is to consider it added.";
            }
        }
    }
    deviceList = Solid::Device::listFromType( Solid::DeviceInterface::StorageDrive );
    for( const Solid::Device &device : deviceList )
    {
        debug() << "Found Solid::DeviceInterface::StorageDrive with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();

        if( device.as<Solid::StorageDrive>() )
        {
            m_type[device.udi()] = MediaDeviceCache::SolidGenericType;
            m_name[device.udi()] = device.vendor() + QStringLiteral(" - ") + device.product();
        }
    }
    deviceList = Solid::Device::listFromType( Solid::DeviceInterface::OpticalDisc );
    for( const Solid::Device &device : deviceList )
    {
        debug() << "Found Solid::DeviceInterface::OpticalDisc with udi = " << device.udi();
        debug() << "Device name is = " << device.product() << " and was made by " << device.vendor();

        const Solid::OpticalDisc * opt = device.as<Solid::OpticalDisc>();

        if ( opt && opt->availableContent() & Solid::OpticalDisc::Audio )
        {
            debug() << "device is an Audio CD";
            m_type[device.udi()] = MediaDeviceCache::SolidAudioCdType;
            m_name[device.udi()] = device.vendor() + QStringLiteral(" - ") + device.product();
        }
    }
    KConfigGroup config = Amarok::config( QStringLiteral("PortableDevices") );
    const QStringList manualDeviceKeys = config.entryMap().keys();
    for( const QString &udi : manualDeviceKeys )
    {
        if( udi.startsWith( QStringLiteral("manual") ) )
        {
            debug() << "Found manual device with udi = " << udi;
            m_type[udi] = MediaDeviceCache::ManualType;
            m_name[udi] = udi.split( QLatin1Char('|') )[1];
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

    Solid::OpticalDisc * opt = device.as<Solid::OpticalDisc>();

    if ( opt && opt->availableContent() & Solid::OpticalDisc::Audio )
    {
        debug() << "device is an Audio CD";
        m_type[udi] = MediaDeviceCache::SolidAudioCdType;
        m_name[udi] = device.vendor() + QStringLiteral(" - ") + device.product();
    }
    else if( ssa )
    {
        debug() << "volume is generic storage";
        if( !m_volumes.contains( device.udi() ) )
        {
            connect( ssa, &Solid::StorageAccess::accessibilityChanged,
                this, &MediaDeviceCache::slotAccessibilityChanged );
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
    else if( device.is<Solid::StorageDrive>() )
    {
        debug() << "device is a Storage drive, still need a volume";
        m_type[udi] = MediaDeviceCache::SolidGenericType;
        m_name[udi] = device.vendor() + QStringLiteral(" - ") + device.product();
    }
    else if( device.is<Solid::PortableMediaPlayer>() )
    {
        debug() << "device is a PMP";
        m_type[udi] = MediaDeviceCache::SolidPMPType;
        m_name[udi] = device.vendor() + QStringLiteral(" - ") + device.product();
    }
    else if( const Solid::GenericInterface *generic = device.as<Solid::GenericInterface>() )
    {
        const QMap<QString, QVariant> properties = generic->allProperties();
        /* At least iPod touch 3G and iPhone 3G do not advertise AFC (Apple File
         * Connection) capabilities. Therefore we have to white-list them so that they are
         * still recognised ad iPods
         *
         * @see IpodConnectionAssistant::identify() for a quirk that is currently also
         * needed for proper identification of iPhone-like devices.
         */
        if ( !device.product().contains( QStringLiteral("iPod" )) && !device.product().contains( QStringLiteral("iPhone") ))
        {
            if( !properties.contains(QStringLiteral("info.capabilities")) )
            {
                debug() << "udi " << udi << " does not describe a portable media player or storage volume";
                return;
            }
    
            const QStringList capabilities = properties[QStringLiteral("info.capabilities")].toStringList();
            if( !capabilities.contains(QStringLiteral("afc")) )
            {
                debug() << "udi " << udi << " does not describe a portable media player or storage volume";
                return;
            }
        }

        debug() << "udi" << udi << "is AFC capable (Apple mobile device)";
        m_type[udi] = MediaDeviceCache::SolidGenericType;
        m_name[udi] = device.vendor() + QStringLiteral(" - ") + device.product();
    }
    else
    {
        debug() << "udi " << udi << " does not describe a portable media player or storage volume";
        return;
    }
    Q_EMIT deviceAdded( udi );
}

void
MediaDeviceCache::slotRemoveSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    debug() << "udi is: " << udi;
    Solid::Device device( udi );
    if( m_volumes.contains( udi ) )
    {
        disconnect( device.as<Solid::StorageAccess>(), &Solid::StorageAccess::accessibilityChanged,
                    this, &MediaDeviceCache::slotAccessibilityChanged );
        m_volumes.removeAll( udi );
        Q_EMIT deviceRemoved( udi );
    }
    if( m_type.contains( udi ) )
    {
        m_type.remove( udi );
        m_name.remove( udi );
        Q_EMIT deviceRemoved( udi );
        return;
    }
    debug() << "Odd, got a deviceRemoved at udi " << udi << " but it did not seem to exist in the first place...";
    Q_EMIT deviceRemoved( udi );
}

void
MediaDeviceCache::slotAccessibilityChanged( bool accessible, const QString &udi )
{
    debug() << "accessibility of device " << udi << " has changed to accessible = " << (accessible ? "true":"false");
    if( accessible )
    {
        Solid::Device device( udi );
        m_type[udi] = MediaDeviceCache::SolidVolumeType;
        Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
        if( ssa )
            m_name[udi] = ssa->filePath();
        Q_EMIT deviceAdded( udi );
        return;
    }
    else
    {
        if( m_type.contains( udi ) )
        {
            m_type.remove( udi );
            m_name.remove( udi );
            Q_EMIT deviceRemoved( udi );
            return;
        }
        debug() << "Got accessibility changed to false but was not there in the first place...";
    }

    Q_EMIT accessibilityChanged( accessible, udi );
}

MediaDeviceCache::DeviceType
MediaDeviceCache::deviceType( const QString &udi ) const
{
    if( m_type.contains( udi ) )
    {
        return m_type[udi];
    }
    return MediaDeviceCache::InvalidType;
}

const QString
MediaDeviceCache::deviceName( const QString &udi ) const
{
    if( m_name.contains( udi ) )
    {
        return m_name[udi];
    }
    return QStringLiteral("ERR_NO_NAME"); //Should never happen!
}

const QString
MediaDeviceCache::device( const QString &udi ) const
{
    DEBUG_BLOCK
    Solid::Device device( udi );
    Solid::Device parent( device.parent() );
    if( !parent.isValid() )
    {
        debug() << udi << "has no parent, returning null string.";
        return QString();
    }

    Solid::Block* sb = parent.as<Solid::Block>();
    if( !sb  )
    {
        debug() << parent.udi() << "failed to convert to Block, returning null string.";
        return QString();
    }

    return sb->device();
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
    if( QFile::exists( ssa->filePath() + QLatin1Char('/') + QStringLiteral(".is_audio_player") ) )
    {
        return true;
    }
    return false;
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


