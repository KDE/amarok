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

UpnpQueryMaker::UpnpQueryMaker( UpnpSearchCollection *collection )
    : QueryMaker()
    , m_collection( collection )
    , m_queryType( None )
    , m_albumMode( AllAlbums )
    , m_asDataPtrs( false )
{
}

UpnpQueryMaker::~UpnpQueryMaker()
{
}


QueryMaker* UpnpQueryMaker::reset()
{
    return this;
}

void UpnpQueryMaker::run()
{
    debug() << "Running";
    switch( m_queryType ) {
        case Artist:
        {
            debug() << "Running for artists";
            KIO::ListJob *job = KIO::listDir( m_collection->collectionId() + "?search=1&query=upnp:class derivedfrom \"object.container.person.musicArtist\"" );
            connect( job, SIGNAL( entries( KIO::Job *, const KIO::UDSEntryList & ) ),
                     this, SLOT( slotEntries( KIO::Job *, const KIO::UDSEntryList & ) ) );
            connect( job, SIGNAL( result(KJob *) ), this, SIGNAL( queryDone() ) );
            debug() << "RUNNING WITH ARTISTS";
        }
        default:
        {
            QStringList l;
            l << "42";
            emit newResultReady( QString(), l );
        }
    }
}

void UpnpQueryMaker::abortQuery()
{
DEBUG_BLOCK
}

QueryMaker* UpnpQueryMaker::setQueryType( QueryType type )
{
DEBUG_BLOCK
    debug() << "Query type" << type;
    m_queryType = type;
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
    debug() << "Add return value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
DEBUG_BLOCK
    debug() << "Return function with value" << value;
    return this;
}

QueryMaker* UpnpQueryMaker::orderBy( qint64 value, bool descending )
{
DEBUG_BLOCK
    debug() << "Order by " << value << "Descending?" << descending;
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
    debug() << "Including collection" << collectionId;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeCollection( const QString &collectionId )
{
DEBUG_BLOCK
    debug() << "Excluding collection" << collectionId;
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::TrackPtr &track )
{
DEBUG_BLOCK
    debug() << "Adding track match" << track->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ArtistPtr &artist )
{
DEBUG_BLOCK
    debug() << "Adding artist match" << artist->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
DEBUG_BLOCK
    debug() << "Adding album match" << album->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
DEBUG_BLOCK
    debug() << "Adding composer match" << composer->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
DEBUG_BLOCK
    debug() << "Adding genre match" << genre->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::YearPtr &year )
{
DEBUG_BLOCK
    debug() << "Adding year match" << year->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::DataPtr &data )
{
DEBUG_BLOCK
    debug() << "Adding dataptr match" << data->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::LabelPtr &label )
{
DEBUG_BLOCK
    debug() << "Adding label match" << label->name();
    return this;
}

QueryMaker* UpnpQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << "Adding filter" << value << filter << matchBegin << matchEnd;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << "Excluding filter" << value << filter << matchBegin << matchEnd;
    return this;
}

QueryMaker* UpnpQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    debug() << "Adding number filter" << value << filter << compare;
    return this;
}

QueryMaker* UpnpQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    debug() << "Excluding number filter" << value << filter << compare;
    return this;
}

QueryMaker* UpnpQueryMaker::limitMaxResultSize( int size )
{
DEBUG_BLOCK
    debug() << "Limit max results to" << size;
    return this;
}

QueryMaker* UpnpQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
DEBUG_BLOCK
    debug() << "Set album query mode" << mode;
    m_albumMode = mode;
    return this;
}

QueryMaker* UpnpQueryMaker::setLabelQueryMode( LabelQueryMode mode )
{
DEBUG_BLOCK
    debug() << "Set label query mode" << mode;
    return this;
}

QueryMaker* UpnpQueryMaker::beginAnd()
{
DEBUG_BLOCK
    debug() << "Begin AND";
    return this;
}

QueryMaker* UpnpQueryMaker::beginOr()
{
DEBUG_BLOCK
    debug() << "Begin OR";
    return this;
}

QueryMaker* UpnpQueryMaker::endAndOr()
{
DEBUG_BLOCK
    debug() << "End AND/OR";
    return this;
}

QueryMaker* UpnpQueryMaker::setAutoDelete( bool autoDelete )
{
DEBUG_BLOCK
    debug() << "Auto delete";
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
    debug() << "SLOT ENTRIES";
    if( m_queryType != Artist ) {
        emit queryDone();
        return;
    }
    // for now just assume artists
    if( m_asDataPtrs ) {
        Meta::DataList ret;
        foreach( KIO::UDSEntry entry, list ) {
            debug() << "ENTRY" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
            Meta::UpnpArtist *artist = new Meta::UpnpArtist( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
            Meta::DataPtr ptr( artist );
            ret << ptr;
        }
        emit newResultReady( m_collection->collectionId(), ret );
    }
    else {
        Meta::ArtistList ret;
        foreach( KIO::UDSEntry entry, list ) {
            debug() << "ENTRY" << entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME );
            Meta::UpnpArtist *artist = new Meta::UpnpArtist( entry.stringValue( KIO::UDSEntry::UDS_DISPLAY_NAME ) );
            Meta::ArtistPtr ptr( artist );
            ret << ptr;
        }
        emit newResultReady( m_collection->collectionId(), ret );
    }

}
} //namespace Collections
