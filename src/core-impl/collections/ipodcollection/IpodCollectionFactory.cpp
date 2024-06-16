/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz                                       *
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

#include "IpodCollectionFactory.h"

#include "IpodCollection.h"
#include "core/support/Debug.h"

#include <solid/device.h>
#include <solid/devicenotifier.h>
#include <solid/portablemediaplayer.h>
#include <solid/storageaccess.h>
#include <solid/storagevolume.h>

#include <QDir>

IpodCollectionFactory::IpodCollectionFactory()
    : CollectionFactory()
{
}


IpodCollectionFactory::~IpodCollectionFactory()
{
}

void
IpodCollectionFactory::init()
{
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded,
             this, &IpodCollectionFactory::slotAddSolidDevice );
    connect( Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved,
             this, &IpodCollectionFactory::slotRemoveSolidDevice );

    // detect iPods that were already connected on startup
    QString query( "[IS StorageAccess OR IS PortableMediaPlayer]" );
    QList<Solid::Device> ipodDevices = Solid::Device::listFromQuery( query );
    for( const Solid::Device &device : ipodDevices )
    {
        if( identifySolidDevice( device.udi() ) )
            createCollectionForSolidDevice( device.udi() );
    }
    m_initialized = true;
}

void
IpodCollectionFactory::slotAddSolidDevice( const QString &udi )
{
    if( m_collectionMap.contains( udi ) )
        return; // a device added twice (?)

    if( identifySolidDevice( udi ) )
        createCollectionForSolidDevice( udi );
}

void
IpodCollectionFactory::slotAccessibilityChanged( bool accessible, const QString &udi )
{
    if( accessible )
        slotAddSolidDevice( udi );
    else
        slotRemoveSolidDevice( udi );
}

void
IpodCollectionFactory::slotRemoveSolidDevice( const QString &udi )
{
    IpodCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotDestroy();
}

void
IpodCollectionFactory::slotCollectionDestroyed( QObject *collection )
{
    // remove destroyed collection from m_collectionMap
    QMutableMapIterator<QString, IpodCollection *> it( m_collectionMap );
    while( it.hasNext() )
    {
        it.next();
        if( (QObject *) it.value() == collection )
            it.remove();
    }
}

/**
 * @brief Return true if device is identified as iPod-compatible using product and vendor.
 *
 * @param device Solid device to identify
 * @return true if the device is iPod-like, false if it cannot be proved.
 **/
static bool
deviceIsRootIpodDevice( const Solid::Device &device )
{
    if( !device.vendor().contains( "Apple", Qt::CaseInsensitive ) )
        return false;
    return device.product().startsWith( "iPod" )
        || device.product().startsWith( "iPhone" )
        || device.product().startsWith( "iPad" );
}

/**
 * @brief Returns true if device is identified as iPod-compatible using
 * PortableMediaPlayer interface.
 *
 * @param device Solid device to identify
 * @return true if the device is iPod-like, false if it cannot be proved.
 **/
static bool
deviceIsPMPIpodDevice( const Solid::Device &device )
{
    /* This should be the one and only way to identify iPod-likes, but as of KDE 4.9.4,
     * solid attaches PortableMediaPlayer to a wrong device path like
     * /org/kde/solid/udev/sys/devices/pci0000:00/0000:00:1a.0/usb1/1-1/1-1.1/1-1.1.4/1-1.1.4:1.0/host6/target6:0:0/6:0:0:0/block/sdc/sdc2,
     * while it should attach is to UDIsks device /org/freedesktop/UDisks/devices/sdc2
     *
     * It works correctly for iPhones though.
     */
    const Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();
    if( !pmp )
        return false;

    debug() << "Device supported PMP protocols:" << pmp->supportedProtocols();
    return pmp->supportedProtocols().contains( "ipod", Qt::CaseInsensitive );
}

bool
IpodCollectionFactory::identifySolidDevice( const QString &udi ) const
{
    DEBUG_BLOCK
    Solid::Device device( udi );

    if( deviceIsPMPIpodDevice( device ) )
    {
        debug() << "Device" << device.udi() << "identified iPod-like using "
                   "PortableMediaPlayer interface";
        return true;
    }

    if( !device.is<Solid::StorageAccess>() )
    {
        debug() << "Device" << device.udi() << "doesn't have PortableMediaPlayer ipod"
                << "interface or StorageAccess interface -> cannot be and iPod";
        return false;
    }

    /* Start with device to identify, opportunistically try to identify it as
     * iPod-compatible. If found not, try its parent. Repeat until parent device is
     * valid.
     *
     * @see MediaDeviceCache::slotAddSolidDevice(), whose iPhone hack shouldn't be
     * needed since iPod rewrite in Amarok 2.6.
     */
    while ( device.isValid() )
    {
        if( deviceIsRootIpodDevice( device ) )
        {
            debug() << "Device" << device.udi() << "identified iPod-like using "
                       "vendor and product name";
            return true;
        }

        debug() << "Device" << device.udi() << "not identified iPod-like, trying parent device";
        device = device.parent();
    }
    debug() << "Device" << device.udi() << "is invalid, returning false. (i.e. was not iPod-like)";
    return false;
}

void
IpodCollectionFactory::createCollectionForSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    DeviceType type;
    QDir mountPoint;
    QString uuid;
    Solid::Device device( udi );
    Solid::StorageAccess *ssa = device.as<Solid::StorageAccess>();
    if( ssa )
    {
        type = iPod;
        if( ssa->isIgnored() )
        {
            debug() << "device" << udi << "ignored, ignoring :-)";
            return;
        }

        // we are definitely interested in this device, listen for accessibility changes
        disconnect( ssa, &Solid::StorageAccess::accessibilityChanged, this, nullptr );
        connect( ssa, &Solid::StorageAccess::accessibilityChanged,
                 this, &IpodCollectionFactory::slotAccessibilityChanged );

        if( !ssa->isAccessible() )
        {
            debug() << "device" << udi << "not accessible, ignoring for now";
            return;
        }
        mountPoint.setPath( ssa->filePath() );
        Solid::StorageVolume *volume = device.as<Solid::StorageVolume>();
        if( volume )
            uuid = volume->uuid();
    }
    else // no ssa
    {
        do { // break inside this block means "continue with collection creation"
            type = iOS;
            debug() << "device" << udi << "has no StorageAccess interface, treating as iPhone/iPad";
            Solid::PortableMediaPlayer *pmp = device.as<Solid::PortableMediaPlayer>();
            if( !pmp )
            {
                debug() << "Ignoring above device as it doesn't have PortableMediaPlayer interface";
                return;
            }

            if( pmp->supportedProtocols().contains( "ipod" ) &&
                pmp->supportedDrivers().contains( "usbmux" ) )
            {
                uuid = pmp->driverHandle( "usbmux" ).toString();
                debug() << "Above device supports ipod/usbmux protocol/driver combo, good";
                break;
            }

            debug() << "Ignoring above device as it doesn't support ipod/usbmux"
                    << "PortableMediaPlayer protocol/driver combo";
            return;
        } while( false );
    }

    debug() << "Creating iPod collection, mount-point (empty if iOS):" << mountPoint
            << "uuid:" << uuid;
    IpodCollection *collection;
    switch( type )
    {
        case iPod:
            collection = new IpodCollection( mountPoint, uuid );
            break;
        case iOS:
            collection = new IpodCollection( uuid );
            break;
    }
    m_collectionMap.insert( udi, collection );

    // when the collection is destroyed by someone else, remove it from m_collectionMap:
    connect( collection, &QObject::destroyed, this, &IpodCollectionFactory::slotCollectionDestroyed );

    if( ssa )
        // try to gracefully destroy collection when unmounting is requested using
        // external means: Device notifier plasmoid etc.. Because the original action
        // could fail if we hold some files on the device open, we eject the collection,
        // not just destroy it.
        connect( ssa, &Solid::StorageAccess::teardownRequested, collection, &IpodCollection::slotEject );

    if( collection->init() )
        Q_EMIT newCollection( collection );
    else
        collection->deleteLater();
}
