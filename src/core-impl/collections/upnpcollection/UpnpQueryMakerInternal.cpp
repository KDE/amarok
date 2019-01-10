/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#define DEBUG_PREFIX "UpnpQueryMakerInternal"

#include "UpnpQueryMakerInternal.h"

#include "upnptypes.h"

#include <QUrlQuery>

#include <KIO/ListJob>
#include <KIO/Scheduler>
#include <KIO/StatJob>

#include "UpnpSearchCollection.h"
#include "UpnpCache.h"
#include "UpnpMeta.h"
#include "core/support/Debug.h"

namespace Collections {

// use filter for faster data transfer and parsing
// if cached tracks > remote tracks * CACHE_CHECK_THRESHOLD
static const float CACHE_CHECK_THRESHOLD = 0.75f;

UpnpQueryMakerInternal::UpnpQueryMakerInternal( UpnpSearchCollection *collection )
    : m_collection( collection )
{
    reset();
}

void UpnpQueryMakerInternal::reset()
{
    m_queryType = QueryMaker::None;
    m_jobCount = 0;
}

UpnpQueryMakerInternal::~UpnpQueryMakerInternal()
{
}

void UpnpQueryMakerInternal::queueJob(KIO::SimpleJob* job)
{
    QUrl url = job->url();
    debug() << "+-+- RUNNING JOB WITH" << url.toDisplayString();
    m_collection->addJob( job );
    m_jobCount++;
    job->start();
}

void UpnpQueryMakerInternal::runQuery( QUrl query, bool filter )
{
    // insert this query as a job
    // first check cache size vs remote size
    // if over threshold, apply filter, otherwise pass on as normal
    int remoteCount = m_collection->property( "numberOfTracks" ).toInt();
    debug() << "REMOTE COUNT" << remoteCount << "Cache size" << m_collection->cache()->tracks().size();
    if( m_collection->cache()->tracks().size() > remoteCount * CACHE_CHECK_THRESHOLD
        && remoteCount > 0
        && filter ) {
        debug() << "FILTERING BY CLASS ONLY";
        QUrlQuery q( query );
        q.addQueryItem( "filter", "upnp:class" );
        query.setQuery( q );
    }

    KIO::ListJob *job = KIO::listDir( query, KIO::HideProgressInfo );
    connect( job, &KIO::ListJob::entries,
             this, &UpnpQueryMakerInternal::slotEntries );
    connect( job, &KJob::result, this, &UpnpQueryMakerInternal::slotDone );
    queueJob( job );
}

void UpnpQueryMakerInternal::runStat( const QString& id )
{
    QUrl url( m_collection->collectionId() );
    QUrlQuery query( url );
    query.addQueryItem( "id", id );
    url.setQuery( query );
    debug() << "STAT URL" << url;
    KIO::StatJob *job = KIO::stat( url, KIO::HideProgressInfo );
    connect( job, &KJob::result, this, &UpnpQueryMakerInternal::slotStatDone );
    queueJob( job );
}

void UpnpQueryMakerInternal::slotEntries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    debug() << "+-+- JOB DONE" << static_cast<KIO::SimpleJob*>(job)->url() << job->error();
    foreach( const KIO::UDSEntry &entry, list )
        debug() << "GOT ENTRY " << entry.stringValue( KIO::UDSEntry::UDS_NAME );
    // actually iterate over the list, check for cache hits
    // if hit, get the relevant Meta::*Ptr object and pass it on
    // for post filtering etc.
    // if miss, queue up another job to fetch all details
    // job->url() can be used to decide if complete details
    // are available or not using queryItem("filter") details
    //
    if( job->error() )
        Q_EMIT results( true, KIO::UDSEntryList() );
    else
        Q_EMIT results( false, list );

    debug() << this << "SLOT ENTRIES" << list.length() << m_queryType;

    switch( m_queryType ) {
        case QueryMaker::Artist:
            handleArtists( list );
            break;
        case QueryMaker::Album:
            handleAlbums( list );
            break;
        case QueryMaker::Track:
            handleTracks( list );
            break;
        case QueryMaker::Custom:
            handleCustom( list );
            break;
        default:
            break;
    // TODO handle remaining cases
    }

    if( !list.empty() ) {
        debug() << "_______________________       RESULTS!  ____________________________";
    }
}

void UpnpQueryMakerInternal::slotDone( KJob *job )
{
    // here check if all jobs done, then we might want to Q_EMIT done()
    // clean up this job, remove it from the hash and so on.
    m_jobCount--;
    job->deleteLater();

    if( m_jobCount <= 0 ) {
        //Q_EMIT newResultReady( list );
        debug() << "ALL JOBS DONE< TERMINATING THIS QM" << this;
        Q_EMIT done();
    }
}

void UpnpQueryMakerInternal::slotStatDone( KJob *job )
{
    m_jobCount--;
    KIO::StatJob *sj = static_cast<KIO::StatJob*>( job );
    if( sj->error() ) {
        debug() << "STAT ERROR ON" << sj->url() << sj->errorString();
    }
    else {
        KIO::UDSEntry entry = sj->statResult();
        slotEntries( static_cast<KIO::Job*>( job ), KIO::UDSEntryList() << entry );
    }
    sj->deleteLater();
    if( m_jobCount <= 0 ) {
        //Q_EMIT newResultReady( list );
        debug() << "ALL JOBS DONE< TERMINATING THIS QM" << this;
        Q_EMIT done();
    }
}

void UpnpQueryMakerInternal::handleArtists( const KIO::UDSEntryList &list )
{
    Meta::ArtistList ret;
    foreach( const KIO::UDSEntry &entry, list ) {
        if( entry.stringValue( KIO::UPNP_CLASS ) == "object.container.person.musicArtist" ) {
            debug() << this << "ARTIST" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
            ret << m_collection->cache()->getArtist( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
        }
        else {
            if( entry.contains( KIO::UPNP_ARTIST ) ) {
                ret << m_collection->cache()->getArtist( entry.stringValue( KIO::UPNP_ARTIST ) );
            }
            else {
                runStat( entry.stringValue( KIO::UPNP_ID ) );
            }
        }
    }
    Q_EMIT newArtistsReady( ret );
}

void UpnpQueryMakerInternal::handleAlbums( const KIO::UDSEntryList &list )
{
DEBUG_BLOCK
    debug() << "HANDLING ALBUMS" << list.length();
    Meta::AlbumList ret;
    foreach( const KIO::UDSEntry &entry, list ) {
        if( entry.stringValue( KIO::UPNP_CLASS ) == "object.container.album.musicAlbum" ) {
            debug() << this << "ALBUM" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) << entry.stringValue(KIO::UPNP_ARTIST);
            ret << m_collection->cache()->getAlbum( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ), entry.stringValue( KIO::UPNP_ARTIST ) );
        }
        else {
            if( entry.contains( KIO::UPNP_ALBUM ) ) {
                ret << m_collection->cache()->getAlbum( entry.stringValue( KIO::UPNP_ALBUM ), entry.stringValue( KIO::UPNP_ARTIST ) );
            }
            else {
                runStat( entry.stringValue( KIO::UPNP_ID ) );
            }
        }
    }
    Q_EMIT newAlbumsReady( ret );
}

void UpnpQueryMakerInternal::handleTracks( const KIO::UDSEntryList &list )
{
DEBUG_BLOCK
    debug() << "HANDLING TRACKS" << list.length();
    Meta::TrackList ret;
    foreach( const KIO::UDSEntry &entry, list ) {
        // If we did a list job with an attempt to check the cache (ie. no meta-data requested from server )
        // we might have an incomplete cache entry for the track.
        // if we have an incomplete entry, we queue a stat job which fetches
        // the entry. Now this stat job is going to call handleTracks again
        // When called from a StatJob, we want to fill up the cache
        // with valid values.
        // So if the cache entry is incomplete, but the UDSEntry is complete
        // set the refresh option to true when calling getTrack
        Meta::TrackPtr track = m_collection->cache()->getTrack( entry );
        if( track->playableUrl().isEmpty() ) {
            debug() << "TRACK HAS INCOMPLETE ENTRY" << track->name() << track->album()->name();
            if( !entry.stringValue( KIO::UDSEntry::UDS_TARGET_URL ).isEmpty() ) {
                debug() << "GOT TRACK DETAILS FROM STAT JOB";
                // reached from a StatJob
                // fill up valid values AND add this track to ret since it is now valid
                track = m_collection->cache()->getTrack( entry, true );
                debug() << "NOW TRACK DETAILS ARE" << track->name() << track->album()->name();
            }
            else {
                // start a StatJob, but DON'T insert this incomplete entry into ret
                debug() << "FETCHING COMPLETE TRACK DATA" << track->name();
                runStat( entry.stringValue( KIO::UPNP_ID ) );
                continue;
            }
        }
        ret << m_collection->cache()->getTrack( entry );
    }
    Q_EMIT newTracksReady( ret );
}

void UpnpQueryMakerInternal::handleCustom( const KIO::UDSEntryList &list )
{
    Q_EMIT newResultReady( list );
}

}
