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

#define DEBUG_PREFIX "MtpCollection"

#include "MtpCollection.h"
#include "MtpCollectionLocation.h"
#include "MtpMeta.h"

#include "../../../statusbar_ng/StatusBar.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "MediaDeviceMonitor.h"
#include "MemoryQueryMaker.h"

#include <KUrl>

#include <QStringList>

AMAROK_EXPORT_PLUGIN( MtpCollectionFactory )

MtpCollectionFactory::MtpCollectionFactory()
    : CollectionFactory()
{
    // nothing to do
}

MtpCollectionFactory::~MtpCollectionFactory()
{
    // nothing to do
}

void
MtpCollectionFactory::init()
{
    DEBUG_BLOCK

            // connect to the monitor
    
    connect( MediaDeviceMonitor::instance(), SIGNAL( mtpReadyToConnect( const QString &, const QString & ) ),
                     SLOT( mtpDetected( const QString &, const QString & ) ) );
    connect( MediaDeviceMonitor::instance(), SIGNAL( mtpReadyToDisconnect( const QString & ) ),
             SLOT( deviceRemoved( const QString & ) ) );

    connect( MediaDeviceMonitor::instance(), SIGNAL( deviceRemoved( const QString & ) ), SLOT( deviceRemoved( const QString & ) ) );

    
/*
    // NOTE: temporary hack to force connect since applet not enabled in trunk

    connect( MediaDeviceMonitor::instance(), SIGNAL( mtpDetected( const QString &, const QString & ) ),
                     SLOT( mtpDetected( const QString &, const QString & ) ) );
    connect( MediaDeviceMonitor::instance(), SIGNAL( deviceRemoved( const QString & ) ), SLOT( deviceRemoved( const QString & ) ) );


    MediaDeviceMonitor::instance()->checkDevicesForMtp();

*/


    // force refresh to scan for mtp, begin signal/slot process for connection of mtp devices

    


    return;
}

/* Public Slots */

void
MtpCollectionFactory::slotCollectionSucceeded( MtpCollection *coll )
{
    DEBUG_BLOCK
            connect( coll, SIGNAL( collectionDisconnected( const QString &) ),
                     SLOT( slotCollectionDisconnected( const QString & ) ) );
    m_collectionMap.insert( coll->udi(), coll );
    debug() << "Inserted into the collectionMap: " << coll->udi();
    emit newCollection( coll );
    debug() << "emitting new mtp collection";
}

void
MtpCollectionFactory::slotCollectionFailed( MtpCollection *coll )
{
    Q_UNUSED( coll );
    // TODO: deal with failure by triggering appropriate removal
    
}

/* Private Slots */

void
MtpCollectionFactory::mtpDetected( const QString & serial, const QString &udi )
{
    MtpCollection* coll = 0;

    debug() << "Udi is: " << udi;
    debug() << "Udi is in map: " << (m_collectionMap.contains( udi ) ? "true" : "false" );

     if( !m_collectionMap.contains( udi ) )
        {
            // create new collection
               coll = new MtpCollection( serial, udi );
            // connect appropriate signals
               connect( coll, SIGNAL( collectionSucceeded( MtpCollection * ) ),
                        this, SLOT( slotCollectionSucceeded( MtpCollection * ) ) );
               connect( coll, SIGNAL( collectionFailed( MtpCollection * ) ),
                        this, SLOT( slotCollectionFailed( MtpCollection * ) ) );

               // begin signal/slot construction process
               coll->init();
        }
        else
            debug() << "MTP Collection for this device is already made: " << udi;
}

void
MtpCollectionFactory::deviceRemoved( const QString &udi )
{
    DEBUG_BLOCK
            if (  m_collectionMap.contains( udi ) )
    {
        MtpCollection* coll = m_collectionMap[ udi ];
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
MtpCollectionFactory::slotCollectionReady()
{
    DEBUG_BLOCK
            MtpCollection *collection = dynamic_cast<MtpCollection*>(  sender() );
    if (  collection )
    {
        debug() << "emitting mtp collection newcollection";
        emit newCollection(  collection );
    }
}

void
MtpCollectionFactory::slotCollectionDisconnected( const QString & udi)
{
    deviceRemoved( udi );
}

//MtpCollection

MtpCollection::MtpCollection( const QString &serial, const QString &udi )
    : Collection()
    , MemoryCollection()
    , m_serial( serial )
    , m_udi( udi )
    , m_handler( 0 )
{
    DEBUG_BLOCK
    // nothing to do
}

MtpCollection::~MtpCollection()
{
    DEBUG_BLOCK
    debug() << "Freeing handler";
    if( m_handler )
        delete m_handler;
}

void
MtpCollection::init()
{
    DEBUG_BLOCK
    m_handler = new Mtp::MtpHandler( this, this );

    connect( m_handler, SIGNAL( succeeded() ),
             this, SLOT( handlerSucceeded() ) );
    connect( m_handler, SIGNAL( failed() ),
             this, SLOT( handlerFailed() ) );

    m_handler->init( m_serial );
}

void
MtpCollection::copyTrackToDevice( const Meta::TrackPtr &track )
{
    
    
    m_handler->copyTrackToDevice( track );
    return;
}

bool
MtpCollection::deleteTrackFromDevice( const Meta::MtpTrackPtr &track )
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
MtpCollection::removeTrack( const Meta::MtpTrackPtr &track )
{
    DEBUG_BLOCK

    // get pointers
    
    Meta::MtpArtistPtr artist = Meta::MtpArtistPtr::dynamicCast( track->artist() );
    Meta::MtpAlbumPtr album = Meta::MtpAlbumPtr::dynamicCast( track->album() );
    Meta::MtpGenrePtr genre = Meta::MtpGenrePtr::dynamicCast( track->genre() );
    Meta::MtpComposerPtr composer = Meta::MtpComposerPtr::dynamicCast( track->composer() );
    Meta::MtpYearPtr year = Meta::MtpYearPtr::dynamicCast( track->year() );

    // remove track from metadata's tracklists

    debug() << "Artist name: " << artist->name();
    
    artist->remTrack( track );
    album->remTrack( track );
    genre->remTrack( track );
    composer->remTrack( track );
    year->remTrack( track );

    // if empty, get rid of metadata in general

    if( artist->tracks().isEmpty() )
    {
        m_artistMap.remove( artist->name() );
        debug() << "Artist still in artist map: " << ( m_artistMap.contains( artist->name() ) ? "yes" : "no");
        acquireWriteLock();
        setArtistMap( m_artistMap );
        releaseLock();
    }
    if( album->tracks().isEmpty() )
    {
        m_albumMap.remove( album->name() );
        acquireWriteLock();
        setAlbumMap( m_albumMap );
        releaseLock();
    }
    if( genre->tracks().isEmpty() )
    {
        m_genreMap.remove( genre->name() );
        acquireWriteLock();
        setGenreMap( m_genreMap );
        releaseLock();
    }
    if( composer->tracks().isEmpty() )
    {
        m_composerMap.remove( composer->name() );
        acquireWriteLock();
        setComposerMap( m_composerMap );
        releaseLock();
    }
    if( year->tracks().isEmpty() )
    {
        m_yearMap.remove( year->name() );
        acquireWriteLock();
        setYearMap( m_yearMap );
        releaseLock();
    }

    // remove from trackmap

    m_trackMap.remove( track->name() );
}

QString
MtpCollection::getTempFileName( const Meta::MtpTrackPtr track, const QString &tempDir )
{
    QString trackFileName = QString::fromUtf8( track->getMtpTrack()->filename );

    QString filename = tempDir + trackFileName;

    return filename;
}

int
MtpCollection::getTrackToFile( const Meta::MtpTrackPtr track, const QString & filename )
{
    return m_handler->getTrackToFile( track->id(), filename );
}

void
MtpCollection::updateTags( Meta::MtpTrack *track)
{
    DEBUG_BLOCK
    Meta::MtpTrackPtr trackPtr( track );
    //KUrl trackUrl = KUrl::fromPath( trackPtr->url() );

    debug() << "Running updateTrackInDB...";

    m_handler->updateTrackInDB( trackPtr );

}

void
MtpCollection::writeDatabase()
{
    // NOTE: NYI, possibly unnecessary or different implementation required
//    m_handler->writeITunesDB( false );
}

void
MtpCollection::deviceRemoved()
{
    emit remove();
}

void
MtpCollection::startFullScan()
{
    //ignore
}

QueryMaker*
MtpCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
MtpCollection::collectionId() const
{
     return m_udi;
}

CollectionLocation*
MtpCollection::location() const
{
    return new MtpCollectionLocation( this );
}

QString
MtpCollection::prettyName() const
{
    // TODO: there's nothing pretty about this name, get a prettier one
    return m_handler->prettyName();
}

QString
MtpCollection::udi() const
{
    return m_udi;
}

void
MtpCollection::setTrackToDelete( const Meta::MtpTrackPtr &track )
{
    m_trackToDelete = track;
}

void
MtpCollection::deleteTrackToDelete()
{
    deleteTrackFromDevice( m_trackToDelete );
}

void
MtpCollection::deleteTrackSlot( Meta::MtpTrackPtr track)
{
    deleteTrackFromDevice( track );
}

void
MtpCollection::slotDisconnect()
{
    emit collectionDisconnected( m_udi );
    //emit remove();
}

void
MtpCollection::copyTracksCompleted()
{
    DEBUG_BLOCK

    // nothing to do
    
}

void
MtpCollection::handlerSucceeded()
{
    m_handler->parseTracks();
    The::statusBarNG()->longMessage(
                   i18n( "The MTP device %1 is connected", m_handler->prettyName() ), StatusBarNG::Information );
    emit collectionSucceeded( this );
}

void
MtpCollection::handlerFailed()
{
    emit collectionFailed( this );
}

#include "MtpCollection.moc"

