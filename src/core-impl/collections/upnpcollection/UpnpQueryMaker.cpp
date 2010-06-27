/****************************************************************************************
 UpnpQueryMaker::* Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
 *                                                                                      *
 * This program is free software
{
}
 you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 UpnpQueryMaker::* Foundation
{
}
 either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY
{
}
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UpnpQueryMaker"

#include "UpnpQueryMaker.h"

#include <kdatetime.h>
#include <kio/upnptypes.h>
#include <kio/scheduler.h>
#include <kio/jobclasses.h>

#include "core/support/Debug.h"
#include "UpnpSearchCollection.h"
#include "UpnpMeta.h"

namespace Collections {

bool UpnpQueryMaker::m_runningJob = false;
int UpnpQueryMaker::m_count = 0;
QHash<QString, KIO::ListJob *> UpnpQueryMaker::m_queries = QHash<QString, KIO::ListJob*>();

UpnpQueryMaker::UpnpQueryMaker( UpnpSearchCollection *collection )
    : QueryMaker()
    , m_collection( collection )
{
    m_count++;
    debug() << this << "HAILO";
    reset();
}

UpnpQueryMaker::~UpnpQueryMaker()
{
    m_count--;
    debug() << this << "BYEBYE";
}


QueryMaker* UpnpQueryMaker::reset()
{
    m_queryType = None;
    m_albumMode = AllAlbums;
    m_asDataPtrs = false;
    m_queryString = QString();

// the Amarok Collection Model expects atleast one entry
// otherwise it will harass us continuously for more entries.
// of course due to the poor quality of UPnP servers I've
// had experience with :P, some may not have sub-results
// for something ( they may have a track with an artist, but
// not be able to give any album for it )
    m_noResults = true;
    return this;
}

void UpnpQueryMaker::run()
{
DEBUG_BLOCK
    if( m_queryString.isEmpty() ) {
        emit queryDone();
        return;
    }

    QString url = m_collection->collectionId() + "?search=1&query=" + m_queryString;

    debug() << this << "Running query" << url;

    KIO::ListJob *job = 0;
    if( m_queries.contains( url ) ) {
        debug() << "Already have a running job with the same query";
        job = m_queries[url];
    }
    else {
        job = KIO::listDir( m_collection->collectionId() + "?search=1&query=" + m_queryString, KIO::HideProgressInfo );
        m_queries[url] = job;
    }

    connect( job, SIGNAL( entries( KIO::Job *, const KIO::UDSEntryList & ) ),
             this, SLOT( slotEntries( KIO::Job *, const KIO::UDSEntryList & ) ) );
    connect( job, SIGNAL( result(KJob *) ), this, SLOT( slotDone(KJob *) ) );
    m_queryString = "";
    m_runningJob = true;
}

void UpnpQueryMaker::abortQuery()
{
DEBUG_BLOCK
    Q_ASSERT( false );
// TODO implement this to kill job
}

QueryMaker* UpnpQueryMaker::setQueryType( QueryType type )
{
DEBUG_BLOCK
// TODO allow all, based on search capabilities
// which should be passed on by the factory
    m_queryType = type;
    switch( type ) {
        case Artist:
            debug() << this << "Query type Artist";
            m_queryString = "( upnp:class derivedfrom \"object.container.person.musicArtist\" ) ";
            break;
        case Album:
            debug() << this << "Query type Album";
            m_queryString = "( upnp:class derivedfrom \"object.container.album.musicAlbum\" ) ";
            break;
        case Track:
            debug() << this << "Query type Track";
            break;
        case Genre:
            debug() << this << "Query type Genre";
            break;
        case Custom:
            debug() << this << "Query type Custom";
            break;
        default:
            debug() << this << "Default case: Query type" << type;
            break;
    }
    return this;
}

QueryMaker* UpnpQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
{
DEBUG_BLOCK
    m_asDataPtrs = resultAsDataPtrs;
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnValue( qint64 value )
{
DEBUG_BLOCK
    debug() << this << "Add return value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
DEBUG_BLOCK
    Q_UNUSED( function )
    debug() << this << "Return function with value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::orderBy( qint64 value, bool descending )
{
DEBUG_BLOCK
    debug() << this << "Order by " << value << "Descending?" << descending;
    return this;
}

QueryMaker* UpnpQueryMaker::orderByRandom()
{
DEBUG_BLOCK
    return this;
}

QueryMaker* UpnpQueryMaker::includeCollection( const QString &collectionId )
{
DEBUG_BLOCK
    debug() << this << "Including collection" << collectionId;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeCollection( const QString &collectionId )
{
DEBUG_BLOCK
    debug() << this << "Excluding collection" << collectionId;
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::TrackPtr &track )
{
DEBUG_BLOCK
    debug() << this << "Adding track match" << track->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
DEBUG_BLOCK
    debug() << this << "Adding artist match" << artist->name();
    m_queryString += " and ( upnp:artist = \"" + artist->name() + "\" )";
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
DEBUG_BLOCK
    debug() << this << "Adding album match" << album->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
DEBUG_BLOCK
    debug() << this << "Adding composer match" << composer->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
DEBUG_BLOCK
    debug() << this << "Adding genre match" << genre->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::YearPtr &year )
{
DEBUG_BLOCK
    debug() << this << "Adding year match" << year->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::DataPtr &data )
{
DEBUG_BLOCK
    debug() << this << "Adding dataptr match" << data->name();
    ( const_cast<Meta::DataPtr&>(data) )->addMatchTo( this );
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::LabelPtr &label )
{
DEBUG_BLOCK
    debug() << this << "Adding label match" << label->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << this << "Adding filter" << value << filter << matchBegin << matchEnd;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << this << "Excluding filter" << value << filter << matchBegin << matchEnd;
    return this;
}

QueryMaker* UpnpQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    debug() << this << "Adding number filter" << value << filter << compare;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    debug() << this << "Excluding number filter" << value << filter << compare;
    return this;
}

QueryMaker* UpnpQueryMaker::limitMaxResultSize( int size )
{
DEBUG_BLOCK
    debug() << this << "Limit max results to" << size;
    return this;
}

QueryMaker* UpnpQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
DEBUG_BLOCK
    debug() << this << "Set album query mode" << mode;
    m_albumMode = mode;
    return this;
}

QueryMaker* UpnpQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
DEBUG_BLOCK
    debug() << this << "Set label query mode" << mode;
    return this;
}

QueryMaker* UpnpQueryMaker::beginAnd()
{
DEBUG_BLOCK
    debug() << this << "Begin AND";
    return this;
}

QueryMaker* UpnpQueryMaker::beginOr()
{
DEBUG_BLOCK
    debug() << this << "Begin OR";
    return this;
}

QueryMaker* UpnpQueryMaker::endAndOr()
{
DEBUG_BLOCK
    debug() << this << "End AND/OR";
    return this;
}

QueryMaker* UpnpQueryMaker::setAutoDelete( bool autoDelete )
{
DEBUG_BLOCK
    debug() << this << "Auto delete" << autoDelete;
    return this;
}

int UpnpQueryMaker::validFilterMask()
{
DEBUG_BLOCK
// TODO return based on our collections search capabilities!
    return TitleFilter | AlbumFilter | ArtistFilter | GenreFilter;
}

void UpnpQueryMaker::slotEntries( KIO::Job *job, const KIO::UDSEntryList &list )
{
    if( job->error() ) {
        debug() << this << "JOB has error" << job->errorString();
        return;
    }
    debug() << this << "SLOT ENTRIES" << list.length() << m_queryType;
    if( m_queryType != Artist ) {
        emit queryDone();
        return;
    }

    if( !list.empty() )
        m_noResults = false;
    // for now just assume artists
    if( m_asDataPtrs ) {
        Meta::DataList ret;
        foreach( KIO::UDSEntry entry, list ) {
            debug() << this << "ENTRY" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
            Meta::UpnpArtist *artist = new Meta::UpnpArtist( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
            Meta::DataPtr ptr( artist );
            ret << ptr;
        }
        debug() << "EMITTING";
        emit newResultReady( m_collection->collectionId(), ret );
    }
    else {
        Meta::ArtistList ret;
        foreach( KIO::UDSEntry entry, list ) {
            debug() << this << "ENTRY" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
            Meta::UpnpArtist *artist = new Meta::UpnpArtist( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
            Meta::ArtistPtr ptr( artist );
            ret << ptr;
        }
        emit newResultReady( m_collection->collectionId(), ret );
    }

}

void
UpnpQueryMaker::slotDone( KJob *job )
{
DEBUG_BLOCK
    m_runningJob = false;
    KIO::ListJob *ljob = static_cast<KIO::ListJob*>( job );
    KIO::ListJob *actual = m_queries[QUrl::fromPercentEncoding( ljob->url().prettyUrl().toAscii() )];
    debug() << this << "Terminating job for URL " << ljob->url() << actual;
    if( actual )
        actual->deleteLater();
    m_queries.remove( QUrl::fromPercentEncoding( ljob->url().prettyUrl().toAscii() ) );


    if( m_noResults ) {
        // TODO proper data types not just DataPtr
        Meta::DataList ret;
        Meta::UpnpTrack *fake = new Meta::UpnpTrack( m_collection );
        Meta::DataPtr ptr( fake );
        ret << ptr;
        emit newResultReady( m_collection->collectionId(), ret );
    }
    emit queryDone();
}
} //namespace Collections
