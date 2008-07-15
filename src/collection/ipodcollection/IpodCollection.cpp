/*
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "IpodCollection"

#include "IpodCollection.h"
#include "IpodCollectionLocation.h"
#include "IpodMeta.h"

#include "amarokconfig.h"
#include "Debug.h"

#include "MediaDeviceCache.h"
#include "MemoryQueryMaker.h"

//solid specific includes
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/storageaccess.h>
#include <solid/storagedrive.h>

#include <KUrl>


#include <QStringList>


AMAROK_EXPORT_PLUGIN( IpodCollectionFactory )

IpodCollectionFactory::IpodCollectionFactory()
    : CollectionFactory()
{
    //nothing to do
}

IpodCollectionFactory::~IpodCollectionFactory()
{

}

void
IpodCollectionFactory::init()
{
    DEBUG_BLOCK

    IpodCollection *coll = 0;

    /* Refresh cache */
    MediaDeviceCache::instance()->refreshCache();
    QStringList udiList = MediaDeviceCache::instance()->getAll();

    /* poll udi list for ipod */
    foreach(const QString &udi, udiList )
        {
            /* if iPod found, make collection */
            if( isIpod( udi ) )
            {
                coll = new IpodCollection(MediaDeviceCache::instance()->volumeMountPoint(udi), udi );
                if ( coll )
                {
                    connect( coll, SIGNAL( collectionDisconnected( const QString &) ),
                             SLOT( slotCollectionDisconnected( const QString & ) ) );
                    emit newCollection( coll );
                    debug() << "emitting new ipod collection";
                }
            }

        }

    // connect to device cache so new devices tested for ipod
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceAdded( const QString& ) ),
              SLOT(  deviceAdded( const QString& ) ) );
    connect(  MediaDeviceCache::instance(),  SIGNAL(  deviceRemoved( const QString& ) ),
              SLOT(  deviceRemoved( const QString& ) ) );
    connect(  MediaDeviceCache::instance(), SIGNAL( accessibilityChanged( bool, const QString & ) ),
              SLOT(  slotAccessibilityChanged( bool, const QString & ) ) );


    return;
}

bool
IpodCollectionFactory::isIpod( const QString &udi )
{

    Solid::Device device;

    device = Solid::Device(udi);
    /* going until we reach a vendor, e.g. Apple */
    while ( device.isValid() && device.vendor().isEmpty() )
    {
        device = Solid::Device( device.parentUdi() );
    }

    debug() << "Device udi: " << udi;
    debug() << "Device name: " << MediaDeviceCache::instance()->deviceName(udi);
    debug() << "Mount point: " << MediaDeviceCache::instance()->volumeMountPoint(udi);
    if ( device.isValid() )
    {
        debug() << "vendor: " << device.vendor() << ", product: " << device.product();
    }

    /* if iPod found, return true */
    return (device.product() == "iPod");

}

void
IpodCollectionFactory::deviceAdded(  const QString &udi )
{
    DEBUG_BLOCK

        debug() << "New device added, testing if Ipod";

    IpodCollection *coll = 0;
    /* if iPod found, make collection if udi is new */
    if( isIpod( udi ) )
    {
        if ( m_collectionMap.contains( udi ) )
        {
            debug() << "Ipod collection for this iPod already made!";
            return;
        }

        coll = new IpodCollection(MediaDeviceCache::instance()->volumeMountPoint(udi), udi );
        if ( coll )
        {
            connect( coll, SIGNAL( collectionDisconnected( const QString & ) ),
                     SLOT( slotCollectionDisconnected( const QString & ) ) );
            emit newCollection( coll );
            m_collectionMap.insert( udi, coll );
            debug() << "New Ipod Found, collection created!";
        }
    }

    return;
}

void
IpodCollectionFactory::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
    if (  m_collectionMap.contains( udi ) )
    {
        IpodCollection* coll = m_collectionMap[ udi ];
                if (  coll )
                {
                    m_collectionMap.remove( udi ); // remove from map
                    coll->deviceRemoved();  //collection will be deleted by collectionmanager
                }
                else
                    warning() << "collection already null";
    }
    else
        warning() << "removing non-existent device";

    return;
}

void
IpodCollectionFactory::slotCollectionDisconnected( const QString & udi)
{
    m_collectionMap.remove( udi ); // remove from map
}

void
IpodCollectionFactory::slotAccessibilityChanged( bool accessible, const QString & udi)
{
    DEBUG_BLOCK
        debug() << "Accessibility changed to: " << ( accessible ? "true":"false" );
    if ( !accessible )
        deviceRemoved( udi );
    else
        deviceAdded( udi );

}

void
IpodCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
        IpodCollection *collection = dynamic_cast<IpodCollection*>(  sender() );
    if (  collection )
    {
        debug() << "emitting ipod collection newcollection";
        emit newCollection(  collection );
    }
}



//IpodCollection

IpodCollection::IpodCollection( const QString &mountPoint, const QString &udi )
    : Collection()
    , MemoryCollection()
    , m_mountPoint( mountPoint )
    , m_udi( udi )
    , m_handler( 0 )
{
    DEBUG_BLOCK

    m_handler = new Ipod::IpodHandler( this, m_mountPoint, this );

    m_handler->parseTracks();

    emit collectionReady();
}

void
IpodCollection::copyTrackToDevice( const Meta::TrackPtr &track )
{
    m_handler->copyTrackToDevice( track );
    emit updated();
    return;
}

bool
IpodCollection::deleteTrackFromDevice( const Meta::IpodTrackPtr &track )
{
    DEBUG_BLOCK

        // remove the track from the device
    if ( !m_handler->deleteTrackFromDevice( track ) )
        return false;

    // remove the track from the collection maps too
    removeTrack ( track );

    // inform treeview collection has updated
    emit updated();
    debug() << "deleteTrackFromDevice returning true";
    return true;
}

void
IpodCollection::removeTrack( const Meta::IpodTrackPtr &track )
{
    Meta::IpodArtistPtr::dynamicCast( track->artist() )->remTrack( track );
    Meta::IpodAlbumPtr::dynamicCast( track->album() )->remTrack( track );
    Meta::IpodGenrePtr::dynamicCast( track->genre() )->remTrack( track );
    Meta::IpodComposerPtr::dynamicCast( track->composer() )->remTrack( track );
    Meta::IpodYearPtr::dynamicCast( track->year() )->remTrack( track );
}

void
IpodCollection::updateTags( Meta::IpodTrack *track)
{
    Meta::IpodTrackPtr trackPtr( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->url() );

    debug() << "Running updateTrackInDB...";

    m_handler->updateTrackInDB( trackUrl, Meta::TrackPtr::staticCast( trackPtr ), track->getIpodTrack() );
    
}

void
IpodCollection::writeDatabase()
{
    m_handler->writeITunesDB( false );
}

IpodCollection::~IpodCollection()
{

}

void
IpodCollection::deviceRemoved()
{
    emit remove();
}

void
IpodCollection::startFullScan()
{
    //ignore
}

QueryMaker*
IpodCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
IpodCollection::collectionId() const
{
     return m_mountPoint;
}

CollectionLocation*
IpodCollection::location() const
{
    return new IpodCollectionLocation( this );
}

QString
IpodCollection::prettyName() const
{
    return "Ipod at " + m_mountPoint;
}

QString
IpodCollection::udi() const
{
    return m_udi;
}

void
IpodCollection::setTrackToDelete( const Meta::IpodTrackPtr &track )
{
    m_trackToDelete = track;
}

void
IpodCollection::deleteTrackToDelete()
{
    deleteTrackFromDevice( m_trackToDelete );
}

void
IpodCollection::deleteTrackSlot( Meta::IpodTrackPtr track)
{
    deleteTrackFromDevice( track );
}

void
IpodCollection::slotDisconnect()
{
    emit collectionDisconnected( m_udi );
    emit remove();
}

#include "IpodCollection.moc"

