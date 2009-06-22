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
#include "IpodConnectionAssistant.h"
#include "IpodDeviceInfo.h"
#include "MediaDeviceInfo.h"

#include "meta/capabilities/CollectionCapability.h"
#include "IpodCollectionLocation.h"
#include "IpodMeta.h"
#include "CollectionCapabilityIpod.h"
#include "SvgHandler.h"

#include "amarokconfig.h"
#include "Debug.h"

#include "MemoryQueryMaker.h"

#include <KMessageBox>
#include <KUrl>

AMAROK_EXPORT_PLUGIN( IpodCollectionFactory )

IpodCollectionFactory::IpodCollectionFactory()
    : MediaDeviceCollectionFactory<IpodCollection> ( new IpodConnectionAssistant() )
{
    //nothing to do
}

IpodCollectionFactory::~IpodCollectionFactory()
{
    DEBUG_BLOCK
}

//IpodCollection

IpodCollection::IpodCollection(MediaDeviceInfo* info)
: Collection()
, MemoryCollection()
, m_handler( 0 )
{
    /** Fetch Info needed to construct IpodCollection */
    IpodDeviceInfo *ipodinfo = qobject_cast<IpodDeviceInfo *>( info );

    m_mountPoint = ipodinfo->mountpoint();
    m_udi = ipodinfo->udi();

    m_handler = new Ipod::IpodHandler( this, m_mountPoint, this );

    if( m_handler->succeeded() )
    {
        m_handler->parseTracks();
        emit collectionReady();
    }
}


bool
IpodCollection::possiblyContainsTrack( const KUrl &url ) const
{
    // We could simply check for iPod_Control except that we could actually have multiple ipods connected
    return url.url().startsWith( m_mountPoint ) || url.url().startsWith( "file://" + m_mountPoint );
}

Meta::TrackPtr
IpodCollection::trackForUrl( const KUrl &url )
{
    QString uid = url.url();
    if( uid.startsWith("file://") )
        uid = uid.remove( 0, 7 );
    Meta::TrackPtr ipodTrack = m_trackMap.value( uid );
    return ipodTrack ? ipodTrack : Collection::trackForUrl(url);
}

bool
IpodCollection::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    DEBUG_BLOCK
    switch( type )
    {
        case Meta::Capability::Collection:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
IpodCollection::createCapabilityInterface( Meta::Capability::Type type )
{
    DEBUG_BLOCK
    switch( type )
    {
        case Meta::Capability::Collection:
            return new Meta::CollectionCapabilityIpod( this );
        default:
            return 0;
    }
}

void
IpodCollection::copyTrackListToDevice( const Meta::TrackList tracklist )
{
    DEBUG_BLOCK
    connect( m_handler, SIGNAL( copyTracksDone( bool  ) ),
             SLOT( slotCopyTracksCompleted( bool ) ), Qt::QueuedConnection );
    m_handler->copyTrackListToDevice( tracklist );
}

void
IpodCollection::removeTrack( const Meta::IpodTrackPtr &track )
{
    DEBUG_BLOCK

    // get pointers
    Meta::IpodArtistPtr artist = Meta::IpodArtistPtr::dynamicCast( track->artist() );
    Meta::IpodAlbumPtr album = Meta::IpodAlbumPtr::dynamicCast( track->album() );
    Meta::IpodGenrePtr genre = Meta::IpodGenrePtr::dynamicCast( track->genre() );
    Meta::IpodComposerPtr composer = Meta::IpodComposerPtr::dynamicCast( track->composer() );
    Meta::IpodYearPtr year = Meta::IpodYearPtr::dynamicCast( track->year() );

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

void
IpodCollection::updateTags( Meta::IpodTrack *track )
{
    DEBUG_BLOCK
    Meta::IpodTrackPtr trackPtr( track );
    KUrl trackUrl = KUrl::fromPath( trackPtr->uidUrl() );

    debug() << "Running updateTrackInDB...";

    m_handler->updateTrackInDB( trackUrl, Meta::TrackPtr::staticCast( trackPtr ), track->getIpodTrack() );
}

void
IpodCollection::writeDatabase()
{
    m_handler->writeDatabase();
}

IpodCollection::~IpodCollection()
{
    DEBUG_BLOCK
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
IpodCollection::deleteTracksSlot( Meta::TrackList tracklist )
{
    DEBUG_BLOCK
    connect( m_handler, SIGNAL( deleteTracksDone() ),
                        SLOT( slotDeleteTracksCompleted() ), Qt::QueuedConnection );

    // remove the tracks from the collection maps
    foreach( Meta::TrackPtr track, tracklist )
        removeTrack( Meta::IpodTrackPtr::staticCast( track ) );

    // remove the tracks from the device
    m_handler->deleteTrackListFromDevice( tracklist );

/*
    const QString text( i18nc( "@info", "Do you really want to delete these %1 tracks?", tracklist.count() ) );
    const bool del = KMessageBox::warningContinueCancel(this,
            text,
            QString() ) == KMessageBox::Continue;
*/

    // inform treeview collection has updated
    emit updated();
}

void
IpodCollection::slotDisconnect()
{
    emit collectionDisconnected( m_udi );
    emit remove();
}

void
IpodCollection::slotCopyTracksCompleted( bool success )
{
    DEBUG_BLOCK

    // HACK: write database regardless
    // See note about "success" in IpodHandler::copyTrackListToDevice
    debug() << "Trying to write iTunes database";
    m_handler->writeDatabase();

    // inform collection location that copying is done

    emit copyTracksCompleted( success );

    // inform treeview collection has updated

    emit updated();
}

void
IpodCollection::slotDeleteTracksCompleted()
{
    DEBUG_BLOCK
    debug() << "Trying to write iTunes database";

    m_handler->writeDatabase();

    // inform treeview collection has updated
    emit updated();
}

void
IpodCollection::connectDevice()
{
    m_handler = new Ipod::IpodHandler( this, m_mountPoint, this );

    if( m_handler->succeeded() )
    {
        m_handler->parseTracks();
        emit collectionReady();
    }
}

void
IpodCollection::disconnectDevice()
{
    slotDisconnect();
}

#include "IpodCollection.moc"

