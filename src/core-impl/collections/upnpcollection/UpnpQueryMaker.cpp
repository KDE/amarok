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

#define DEBUG_PREFIX "UpnpQueryMaker"

#include "UpnpQueryMaker.h"

#include <kdatetime.h>
#include "upnptypes.h"
#include <kio/scheduler.h>
#include <kio/jobclasses.h>

#include "core/support/Debug.h"
#include "UpnpSearchCollection.h"
#include "UpnpQueryMakerInternal.h"
#include "UpnpMeta.h"
#include "UpnpCache.h"

namespace Collections {

UpnpQueryMaker::UpnpQueryMaker( UpnpSearchCollection *collection )
    : QueryMaker()
    , m_collection( collection )
    , m_internalQM( new UpnpQueryMakerInternal( collection ) )
{
    reset();
    connect( m_internalQM, SIGNAL(done()), this, SLOT(slotDone()) );

    connect( m_internalQM, SIGNAL(newResultReady(Meta::TrackList)),
             this, SLOT(handleTracks(Meta::TrackList)) );
    connect( m_internalQM, SIGNAL(newResultReady(Meta::ArtistList)),
             this, SLOT(handleArtists(Meta::ArtistList)) );
    connect( m_internalQM, SIGNAL(newResultReady(Meta::AlbumList)),
             this, SLOT(handleAlbums(Meta::AlbumList)) );
    connect( m_internalQM, SIGNAL(newResultReady(KIO::UDSEntryList)),
             this, SLOT(handleCustom(KIO::UDSEntryList)) );
}

UpnpQueryMaker::~UpnpQueryMaker()
{
    m_internalQM->deleteLater();
}


QueryMaker* UpnpQueryMaker::reset()
{
    // TODO kill all jobs here too
    m_queryType = None;
    m_albumMode = AllAlbums;
    m_query.reset();
    m_jobCount = 0;

    m_numericFilters.clear();
    m_internalQM->reset();

// the Amarok Collection Model expects at least one entry
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

    QUrl baseUrl( m_collection->collectionId() );
    baseUrl.addQueryItem( "search", "1" );

    if( m_queryType == Custom ) {
        switch( m_returnFunction ) {
            case Count:
                m_query.reset();
                m_query.setType( "( upnp:class derivedfrom \"object.item.audioItem\" )" );
                baseUrl.addQueryItem( "getCount", "1" );
                break;
            case Sum:
            case Max:
            case Min:
                break;
        }
    }
    // we don't deal with compilations
    else if( m_queryType == Album && m_albumMode == OnlyCompilations ) {
        // we don't support any other attribute
        emit newResultReady( Meta::TrackList() );
        emit newResultReady( Meta::ArtistList() );
        emit newResultReady( Meta::AlbumList() );
        emit newResultReady( Meta::GenreList() );
        emit newResultReady( Meta::ComposerList() );
        emit newResultReady( Meta::YearList() );
        emit newResultReady( QStringList() );
        emit newResultReady( Meta::LabelList() );
        emit queryDone();
        return;
    }

    QStringList queryList;
    if( m_query.hasMatchFilter() || !m_numericFilters.empty() ) {
        queryList = m_query.queries();
    }
    else {
        switch( m_queryType ) {
             case Artist:
                 debug() << this << "Query type Artist";
                 queryList << "( upnp:class derivedfrom \"object.container.person.musicArtist\" )";
                 break;
             case Album:
                 debug() << this << "Query type Album";
                 queryList << "( upnp:class derivedfrom \"object.container.album.musicAlbum\" )";
                 break;
             case Track:
                 debug() << this << "Query type Track";
                 queryList << "( upnp:class derivedfrom \"object.item.audioItem\" )";
                 break;
             case Genre:
                 debug() << this << "Query type Genre";
                 queryList << "( upnp:class derivedfrom \"object.container.genre.musicGenre\" )";
                 break;
             case Custom:
                 debug() << this << "Query type Custom";
                 queryList << "( upnp:class derivedfrom \"object.item.audioItem\" )";
                 break;
             default:
                 debug() << this << "Default case: Query type";
                 // we don't support any other attribute
                 emit newResultReady( Meta::TrackList() );
                 emit newResultReady( Meta::ArtistList() );
                 emit newResultReady( Meta::AlbumList() );
                 emit newResultReady( Meta::GenreList() );
                 emit newResultReady( Meta::ComposerList() );
                 emit newResultReady( Meta::YearList() );
                 emit newResultReady( QStringList() );
                 emit newResultReady( Meta::LabelList() );
                 emit queryDone();
                 return;
        }
    }

    // and experiment in using the filter only for the query
    // and checking the returned upnp:class
    // based on your query types.
    for( int i = 0; i < queryList.length() ; i++ ) {
        if( queryList[i].isEmpty() )
            continue;

        QUrl url( baseUrl );
        url.addQueryItem( "query", queryList[i] );

        debug() << this << "Running query" << url;
        m_internalQM->runQuery( url );
    }
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
    m_query.setType( "( upnp:class derivedfrom \"object.item.audioItem\" )" );
    m_internalQM->setQueryType( type );

    return this;
}

QueryMaker* UpnpQueryMaker::addReturnValue( qint64 value )
{
DEBUG_BLOCK
    debug() << this << "Add return value" << value;
    m_returnValue = value;
    return this;
}

QueryMaker* UpnpQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
{
DEBUG_BLOCK
    Q_UNUSED( function )
    debug() << this << "Return function with value" << value;
    m_returnFunction = function;
    m_returnValue = value;
    return this;
}

QueryMaker* UpnpQueryMaker::orderBy( qint64 value, bool descending )
{
DEBUG_BLOCK
    debug() << this << "Order by " << value << "Descending?" << descending;
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::TrackPtr &track )
{
DEBUG_BLOCK
    debug() << this << "Adding track match" << track->name();
    // TODO: CHECK query type before searching by dc:title?
    m_query.addMatch( "( dc:title = \"" + track->name() + "\" )" );
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ArtistPtr &artist, QueryMaker::ArtistMatchBehaviour behaviour )
{
DEBUG_BLOCK
    Q_UNUSED( behaviour ); // TODO: does UPnP tell between track and album artists?
    debug() << this << "Adding artist match" << artist->name();
    m_query.addMatch( "( upnp:artist = \"" + artist->name() + "\" )" );
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::AlbumPtr &album )
{
DEBUG_BLOCK
    debug() << this << "Adding album match" << album->name();
    m_query.addMatch( "( upnp:album = \"" + album->name() + "\" )" );
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::ComposerPtr &composer )
{
DEBUG_BLOCK
    debug() << this << "Adding composer match" << composer->name();
// NOTE unsupported
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::GenrePtr &genre )
{
DEBUG_BLOCK
    debug() << this << "Adding genre match" << genre->name();
    m_query.addMatch( "( upnp:genre = \"" + genre->name() + "\" )" );
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::YearPtr &year )
{
DEBUG_BLOCK
    debug() << this << "Adding year match" << year->name();
// TODO
    return this;
}

QueryMaker* UpnpQueryMaker::addMatch( const Meta::LabelPtr &label )
{
DEBUG_BLOCK
    debug() << this << "Adding label match" << label->name();
// NOTE how?
    return this;
}

QueryMaker* UpnpQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << this << "Adding filter" << value << filter << matchBegin << matchEnd;

// theoretically this should be '=' I think and set to contains below if required
    QString cmpOp = "contains";
    //TODO should we add filters ourselves
    // eg. we always query for audioItems, but how do we decide
    // whether to add a dc:title filter or others.
    // for example, for the artist list
    // our query should be like ( pseudocode )
    // ( upnp:class = audioItem ) and ( dc:title contains "filter" )
    // OR
    // ( upnp:class = audioItem ) and ( upnp:artist contains "filter" );
    // ...
    // so who adds the second query?
    QString property = propertyForValue( value );
    if( property.isNull() )
        return this;

    if( matchBegin || matchEnd )
        cmpOp = "contains";

    QString filterString = "( " + property + " " + cmpOp + " \"" + filter + "\" ) ";
    m_query.addFilter( filterString );
    return this;
}

QueryMaker* UpnpQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
{
DEBUG_BLOCK
    debug() << this << "Excluding filter" << value << filter << matchBegin << matchEnd;
    QString cmpOp = "!=";
    QString property = propertyForValue( value );
    if( property.isNull() )
        return this;

    if( matchBegin || matchEnd )
        cmpOp = "doesNotContain";

    QString filterString = "( " + property + " " + cmpOp + " \"" + filter + "\" ) ";
    m_query.addFilter( filterString );
    return this;
}

QueryMaker* UpnpQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
{
DEBUG_BLOCK
    debug() << this << "Adding number filter" << value << filter << compare;
    NumericFilter f = { value, filter, compare };
    m_numericFilters << f;
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
    m_query.beginAnd();
    return this;
}

QueryMaker* UpnpQueryMaker::beginOr()
{
DEBUG_BLOCK
    m_query.beginOr();
    return this;
}

QueryMaker* UpnpQueryMaker::endAndOr()
{
DEBUG_BLOCK
    debug() << this << "End AND/OR";
    m_query.endAndOr();
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
    int mask = 0;
    QStringList caps = m_collection->searchCapabilities();
    if( caps.contains( "dc:title" ) )
        mask |= TitleFilter;
    if( caps.contains( "upnp:album" ) )
        mask |= AlbumFilter;
    if( caps.contains( "upnp:artist" ) )
        mask |= ArtistFilter;
    if( caps.contains( "upnp:genre" ) )
        mask |= GenreFilter;
    return mask;
}

void UpnpQueryMaker::handleArtists( Meta::ArtistList list )
{
    // TODO Post filtering
    emit newResultReady( list );
}

void UpnpQueryMaker::handleAlbums( Meta::AlbumList list )
{
    // TODO Post filtering
    emit newResultReady( list );
}

void UpnpQueryMaker::handleTracks( Meta::TrackList list )
{
    // TODO Post filtering
    emit newResultReady( list );
}

/*
void UpnpQueryMaker::handleCustom( const KIO::UDSEntryList& list )
{
    if( m_returnFunction == Count )
    {
        {
        Q_ASSERT( !list.empty() );
        QString count = list.first().stringValue( KIO::UDSEntry::UDS_NAME );
        m_collection->setProperty( "numberOfTracks", count.toUInt() );
        emit newResultReady( QStringList( count ) );
        }
        default:
            debug() << "Custom result functions other than \"Count\" are not supported by UpnpQueryMaker";
    }
}
*/

void UpnpQueryMaker::slotDone()
{
DEBUG_BLOCK
    if( m_noResults ) {
        debug() << "++++++++++++++++++++++++++++++++++++ NO RESULTS ++++++++++++++++++++++++";
        // TODO proper data types not just DataPtr
        Meta::DataList ret;
        Meta::UpnpTrack *fake = new Meta::UpnpTrack( m_collection );
        fake->setTitle( "No results" );
        fake->setYear( Meta::UpnpYearPtr( new Meta::UpnpYear( 2010 ) ) );
        Meta::DataPtr ptr( fake );
        ret << ptr;
        //emit newResultReady( ret );
    }

    switch( m_queryType ) {
        case Artist:
        {
            Meta::ArtistList list;
            foreach( Meta::DataPtr ptr, m_cacheEntries )
                list << Meta::ArtistPtr::staticCast( ptr );
            emit newResultReady( list );
            break;
        }

        case Album:
        {
            Meta::AlbumList list;
            foreach( Meta::DataPtr ptr, m_cacheEntries )
                list << Meta::AlbumPtr::staticCast( ptr );
            emit newResultReady( list );
            break;
        }

        case Track:
        {
            Meta::TrackList list;
            foreach( Meta::DataPtr ptr, m_cacheEntries )
                list << Meta::TrackPtr::staticCast( ptr );
            emit newResultReady( list );
            break;
        }
        default:
        {
            debug() << "Query type not supported by UpnpQueryMaker";
        }
    }

    debug() << "ALL JOBS DONE< TERMINATING THIS QM" << this;
    emit queryDone();
}

QString UpnpQueryMaker::propertyForValue( qint64 value )
{
    switch( value ) {
        case Meta::valTitle:
            return "dc:title";
        case Meta::valArtist:
        {
            //if( m_queryType != Artist )
                return "upnp:artist";
        }
        case Meta::valAlbum:
        {
            //if( m_queryType != Album )
                return "upnp:album";
        }
        case Meta::valGenre:
            return "upnp:genre";
            break;
        default:
            debug() << "UNSUPPORTED QUERY TYPE" << value;
            return QString();
    }
}

bool UpnpQueryMaker::postFilter( const KIO::UDSEntry &entry )
{
    //numeric filters
    foreach( const NumericFilter &filter, m_numericFilters ) {
        // should be set by the filter based on filter.type
        qint64 aValue = 0;

        switch( filter.type ) {
            case Meta::valCreateDate:
            {
                // TODO might use UDSEntry::UDS_CREATION_TIME instead later
                QString dateString = entry.stringValue( KIO::UPNP_DATE );
                QDateTime time = QDateTime::fromString( dateString, Qt::ISODate );
                if( !time.isValid() )
                    return false;
                aValue = time.toTime_t();
                debug() << "FILTER BY creation timestamp entry:" << aValue << "query:" << filter.value << "OP:" << filter.compare;
                break;
            }
        }

        if( ( filter.compare == Equals ) && ( filter.value != aValue ) )
            return false;
        else if( ( filter.compare == GreaterThan ) && ( filter.value >= aValue ) )
            return false; // since only allow entries with aValue > filter.value
        else if( ( filter.compare == LessThan ) && ( filter.value <= aValue ) )
            return false;
    }
    return true;
}

} //namespace Collections
