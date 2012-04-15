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


AMAROK_EXPORT_COLLECTION( IpodCollectionFactory, ipodcollection )

IpodCollectionFactory::IpodCollectionFactory( QObject *parent, const QVariantList &args )
    : CollectionFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_collection-ipodcollection.desktop", "services" );
}

IpodCollectionFactory::~IpodCollectionFactory()
{
}

void
IpodCollectionFactory::init()
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
             SLOT(slotAddSolidDevice(QString)) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
             SLOT(slotRemoveSolidDevice(QString)) );

    // detect iPods that were already connected on startup
    QString query( "[IS StorageAccess OR IS PortableMediaPlayer]" );
    QList<Solid::Device> ipodDevices = Solid::Device::listFromQuery( query );
    foreach( Solid::Device device, ipodDevices )
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
IpodCollectionFactory::slotRemoveAndTeardownSolidDevice( const QString &udi )
{
    IpodCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotEject();
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
    /* This should be the one and only way to identify iPod-likes, but as of KDE 4.7.2,
     * solid does not attach PortableMediaPlayer to iPods at all and its
     * PortableMediaPlayer implementations is just a stub. This would also need
     * media-device-info package to be installed.
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

    /* Start with device to identify, opportunistically try to identify it as
     * iPod-compatible. If found not, try its parent. Repeat until parent device is
     * valid.
     *
     * This method DOES return false positives for iPod-like devices which are itself
     * storage drives, but not storage volumes (such as sdc). This is needed for
     * iPhone-like devices that have extra code to mount them in IpodHandler. IpodHandler
     * gracefully fails in case this is old-school iPod, so this shouldn't hurt, only side
     * effect is that other connection assistant are not tried for these false-identified
     * devices. It would be of course great if iPhone-like devices could be distinguished
     * right here - KDE's Solid should be able to help us with it in future, but as of
     * KDE 4.7.4 it tells us nothing.
     *
     * @see MediaDeviceCache::slotAddSolidDevice() for a quirk that is currently also
     * needed for proper identification of iPhone-like devices. THIS IS CURRENTLY NOT
     * NEEDED FOR IPOD COLLECTION REWRITE, BUT LEFT FOR REFERENCE.
     */
    while ( device.isValid() )
    {
        if( deviceIsPMPIpodDevice( device ) )
        {
            debug() << "Device" << device.udi() << "identified iPod-like using "
                       "PortableMediaPlayer interface";
            return true;
        }
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
    QString mountPointOrUuid;
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
        disconnect( ssa, SIGNAL(accessibilityChanged(bool,QString)), this, 0 );
        connect( ssa, SIGNAL(accessibilityChanged(bool,QString)),
                SLOT(slotAccessibilityChanged(bool,QString)) );

        if( !ssa->isAccessible() )
        {
            debug() << "device" << udi << "not accessible, ignoring for now";
            return;
        }
        mountPointOrUuid = ssa->filePath();
    }
    else // no ssa
    {
        type = iOS;
        debug() << "device" << udi << "has no StorageAccess interface, treating as iPhone/iPad";
        // HACK: in order to avoid many false-positives once KDE's solid attaches PMP
        // interface to class iPods too, check that both PMP and vendor/name matches for
        // "leaf" iPhone device:
        if( !device.is<Solid::PortableMediaPlayer>() || !deviceIsRootIpodDevice( device ) )
        {
            debug() << "Ignoring above device as it either has no PortableMediaPlayer interface or vendor/product doesn't match";
            return;
        }
        // TODO: get device uuid
    }

    debug() << "creating iPod collection, mount-point or uuid:" << mountPointOrUuid;
    IpodCollection *collection;
    switch( type )
    {
        case iPod:
            collection = new IpodCollection( QDir( mountPointOrUuid ) ); // QDir to call correct overload
            break;
        case iOS:
            collection = new IpodCollection( mountPointOrUuid );
            break;
    }
    m_collectionMap.insert( udi, collection );

    // when the collection is destroyed by someone else, remove it from m_collectionMap:
    connect( collection, SIGNAL(destroyed(QObject*)), SLOT(slotCollectionDestroyed(QObject*)) );

    // try to gracefully destroy collection when unmounting is requested using
    // external means: (Device notifier plasmoid etc.). Because the original action could
    // fail if we hold some files on the device open, we try to tearDown the device too.
    connect( ssa, SIGNAL(teardownRequested(QString)), SLOT(slotRemoveAndTeardownSolidDevice(QString)) );

    emit newCollection( collection );  // TODO: emit only when the collection was successfully created
}

#include "IpodCollectionFactory.moc"
