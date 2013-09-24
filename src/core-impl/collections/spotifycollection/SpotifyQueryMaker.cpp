/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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

#include "SpotifyQueryMaker.h"

#include "SpotifyCollection.h"
#include "SpotifyMeta.h"
#include "support/Controller.h"
#include "support/QMFunctionTypes.h"
#include "support/Query.h"

#include "core-impl/collections/support/MemoryQueryMaker.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"

#include <QObject>
#include <QStack>

namespace Collections
{
    SpotifyQueryMaker::SpotifyQueryMaker( SpotifyCollection *collection )
    : m_queryType( QueryMaker::QueryType( None ) )
    , m_autoDelete( false )
    , m_activeQueryCount( 0 )
    , m_memoryQueryIsRunning( false )
    , m_collectionUpdated( false )
    , m_querySent( false )
    , m_filterMap( )
    , m_collection( collection )
    , m_controller( collection->controller() )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = new MemoryQueryMaker( m_collection.data()->memoryCollection().toWeakRef(),
                                                   m_collection.data()->collectionId() );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::TrackList ) ),
                 this, SIGNAL( newResultReady( Meta::TrackList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::ArtistList ) ),
                 this, SIGNAL( newResultReady( Meta::ArtistList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::AlbumList ) ),
                 this, SIGNAL( newResultReady( Meta::AlbumList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::GenreList ) ),
                 this, SIGNAL( newResultReady( Meta::GenreList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::ComposerList ) ),
                 this, SIGNAL( newResultReady( Meta::ComposerList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::YearList ) ),
                 this, SIGNAL( newResultReady( Meta::YearList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::DataList ) ),
                 this, SIGNAL( newResultReady( Meta::DataList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( QStringList ) ),
                 this, SIGNAL( newResultReady( QStringList) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::LabelList ) ),
                 this, SIGNAL( newResultReady( Meta::LabelList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( queryDone() ),
                 this, SLOT( slotMemoryQueryDone() ) );
        m_memoryQueryMaker.data()->setAutoDelete( true );
    }

    SpotifyQueryMaker::~SpotifyQueryMaker()
    {
        DEBUG_BLOCK

        if( m_querySent ){
            warning() << "Query still running when destroying SpotifyQueryMaker...";
        }
        if( !m_queryMakerFunctions.isEmpty() )
        {
            qDeleteAll( m_queryMakerFunctions.begin(), m_queryMakerFunctions.end() );
            m_queryMakerFunctions.clear();
        }

        delete m_memoryQueryMaker.data();

        disconnect( this, SIGNAL( spotifyError( Spotify::Controller::ErrorState ) ),
                    m_collection.data(), SLOT( slotSpotifyError( Spotify::Controller::ErrorState)) );
    }


    void
    SpotifyQueryMaker::run()
    {
        DEBUG_BLOCK

        if( !m_filterMap.isEmpty() )
        {
            connect( m_controller.data(), SIGNAL( spotifyError( const Spotify::Controller::ErrorState ) ),
                     this, SLOT( slotSpotifyError( const Spotify::Controller::ErrorState ) ) );

            QString artist( "" );
            QString album( "" );
            QString title( "" );
            QString genre( "" );

            if( m_filterMap.contains( Meta::valArtist ) )
                artist.append( m_filterMap.value( Meta::valArtist ) );
            if( m_filterMap.contains( Meta::valAlbum ) )
                album.append( m_filterMap.value( Meta::valAlbum ) );
            if( m_filterMap.contains( Meta::valTitle ) )
                title.append( m_filterMap.value( Meta::valTitle ) );
            if( m_filterMap.contains( Meta::valGenre ) )
                genre.append( m_filterMap.value( Meta::valGenre ) );

            if( !artist.isEmpty() || !album.isEmpty() || !title.isEmpty() || !genre.isEmpty() )
            {
                m_querySent = true;
                Spotify::Query* query = m_controller.data()->makeQuery( m_collection.data(), title, artist, album, genre );
                connect( this, SIGNAL( queryAborted() ),
                         query, SLOT( slotAbortQuery()) );
                connect( query, SIGNAL(newTrackList( Meta::SpotifyTrackList ) ),
                         this, SLOT(slotCollectResults( Meta::SpotifyTrackList ) ) );

                m_controller.data()->resolve( query );
            }
        }

        m_activeQueryCount++;
        m_memoryQueryIsRunning = true;
        m_memoryQueryMaker.data()->run();
    }

    void
    SpotifyQueryMaker::abortQuery()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker.data()->abortQuery();
        m_controller.data()->disconnect( this, SLOT(slotCollectResults(Meta::SpotifyTrackList)) );
        m_controller.data()->disconnect( this, SLOT(slotQueryDone(Spotify::Query*,Meta::SpotifyTrackList)) );

        if( m_querySent )
        {
            emit queryAborted();
        }
    }

    QueryMaker*
    SpotifyQueryMaker::setQueryType( QueryType type )
    {
        DEBUG_BLOCK
        CurriedUnaryQMFunction< QueryType >::FunPtr funPtr = &QueryMaker::setQueryType;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< QueryType >( funPtr, type );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        m_queryType = type;

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addReturnValue( qint64 value )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< qint64 >::FunPtr funPtr = &QueryMaker::addReturnValue;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< qint64 >( funPtr, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< ReturnFunction, qint64 >::FunPtr funPtr = &QueryMaker::addReturnFunction;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< ReturnFunction, qint64 >( funPtr, function, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::orderBy( qint64 value, bool descending )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< qint64, bool >::FunPtr funPtr = &QueryMaker::orderBy;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< qint64, bool >( funPtr, value, descending );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::TrackPtr &track )
    {
        DEBUG_BLOCK
        debug() << "Match track: " << track->prettyName();
        CurriedUnaryQMFunction< const Meta::TrackPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::TrackPtr& >( funPtr, track );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behavior )
    {
        DEBUG_BLOCK
        debug() << "Match artist: " << artist->prettyName();
        CurriedBinaryQMFunction< const Meta::ArtistPtr&, ArtistMatchBehaviour >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< const Meta::ArtistPtr&, ArtistMatchBehaviour >( funPtr, artist, behavior );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        if( artist )
            m_filterMap.insert( Meta::valArtist, artist->name() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::AlbumPtr &album )
    {
        DEBUG_BLOCK
        debug() << "Match album: " << album->prettyName();
        CurriedUnaryQMFunction< const Meta::AlbumPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::AlbumPtr& >( funPtr, album );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        if( album )
            m_filterMap.insert( Meta::valAlbum, album->name() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::ComposerPtr &composer )
    {
        DEBUG_BLOCK
        debug() << "Match composer: " << composer->prettyName();
        CurriedUnaryQMFunction< const Meta::ComposerPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::ComposerPtr& >( funPtr, composer );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::GenrePtr &genre )
    {
        DEBUG_BLOCK
        debug() << "Match genre: " << genre->prettyName();
        CurriedUnaryQMFunction< const Meta::GenrePtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::GenrePtr& >( funPtr, genre );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::YearPtr &year )
    {
        DEBUG_BLOCK
        // Currently it will ignore year match
        CurriedUnaryQMFunction< const Meta::YearPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::YearPtr& >( funPtr, year );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addMatch( const Meta::LabelPtr &label )
    {
        DEBUG_BLOCK
        // Current it will ignore label match
        CurriedUnaryQMFunction< const Meta::LabelPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::LabelPtr& >( funPtr, label );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK
        debug() << "Adding filter: " << value << " " << filter;
        CurriedQMStringFilterFunction::FunPtr funPtr = &QueryMaker::addFilter;
        CurriedQMFunction *curriedFun =
            new CurriedQMStringFilterFunction( funPtr, value, filter, matchBegin, matchEnd );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        if( !m_filterMap.isEmpty() && m_filterMap.contains( value ) )
        {
            QString newFilter = m_filterMap.value( value );
            newFilter.append( QString( " " ) ).append( filter );
            m_filterMap.insert( value, newFilter );
        }
        else
            m_filterMap.insert( value, filter );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK

        CurriedQMStringFilterFunction::FunPtr funPtr = &QueryMaker::excludeFilter;
        CurriedQMFunction *curriedFun =
            new CurriedQMStringFilterFunction( funPtr, value, filter, matchBegin, matchEnd );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        if( m_filterMap.contains( value ) && m_filterMap.value( value ).contains( filter ) )
        {
            QString localFilter = m_filterMap.value( value );
            localFilter.remove( filter );
            m_filterMap.insert( value, localFilter );
        }

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
    {
        DEBUG_BLOCK

        CurriedTrinaryQMFunction< qint64, qint64, NumberComparison >::FunPtr funPtr = &QueryMaker::addNumberFilter;
        CurriedQMFunction *curriedFun =
            new CurriedTrinaryQMFunction< qint64, qint64, NumberComparison >
            (
                funPtr, value, filter, compare
            );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
    {
        DEBUG_BLOCK

        CurriedTrinaryQMFunction< qint64, qint64, NumberComparison >::FunPtr funPtr = &QueryMaker::excludeNumberFilter;
        CurriedQMFunction *curriedFun =
            new CurriedTrinaryQMFunction< qint64, qint64, NumberComparison >
            (
                funPtr, value, filter, compare
            );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::limitMaxResultSize( int size )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< int >::FunPtr funPtr = &QueryMaker::limitMaxResultSize;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< int >( funPtr, size );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< AlbumQueryMode >::FunPtr funPtr = &QueryMaker::setAlbumQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< AlbumQueryMode >( funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::setLabelQueryMode( LabelQueryMode mode )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< LabelQueryMode >::FunPtr funPtr = &QueryMaker::setLabelQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< LabelQueryMode >( funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::beginAnd()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginAnd;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::beginOr()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::endAndOr()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::endAndOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        return this;
    }

    QueryMaker*
    SpotifyQueryMaker::setAutoDelete( bool autoDelete )
    {
        DEBUG_BLOCK

        m_autoDelete = autoDelete;

        return this;
    }

    int
    SpotifyQueryMaker::validFilterMask()
    {
        DEBUG_BLOCK

        return QueryMaker::ValidFilters( ArtistFilter ) |
               QueryMaker::ValidFilters( AlbumFilter ) |
               QueryMaker::ValidFilters( TitleFilter ) |
               m_memoryQueryMaker.data()->validFilterMask();
    }

    void
    SpotifyQueryMaker::slotSpotifyError( const Spotify::Controller::ErrorState error )
    {
        DEBUG_BLOCK

        emit spotifyError( error );
    }

    void
    SpotifyQueryMaker::slotCollectResults( const Meta::SpotifyTrackList& trackList )
    {
        DEBUG_BLOCK

        Meta::TrackList list;
        // Add results to collection
        foreach( Meta::SpotifyTrackPtr trackPtr, trackList )
        {
            list.append( Meta::TrackPtr::staticCast( trackPtr ) );
            trackPtr->addToCollection( m_collection.data() );
            if( m_collection.data()->trackForUrl( trackPtr->uidUrl() ) == Meta::TrackPtr::staticCast( trackPtr ) )
                m_collectionUpdated = true;
        }

        emit newResultReady( list );
    }

    void
    SpotifyQueryMaker::slotQueryDone( Spotify::Query *query, const Meta::SpotifyTrackList trackList )
    {
        DEBUG_BLOCK

        Q_UNUSED( query );
        Q_UNUSED( trackList );

        query->disconnect( this, SLOT(spotifyError(Spotify::Controller::ErrorState)) );
        query->disconnect( this, SLOT(slotQueryDone(Spotify::Query*,Meta::SpotifyTrackList)) );

        m_activeQueryCount--;
        m_querySent = false;

        if( m_activeQueryCount <= 0 )
        {
            if( m_collectionUpdated && !m_memoryQueryIsRunning )
            {
                m_collectionUpdated = false;
                runMemoryQueryAgain();
            }
            else
            {
                emit queryDone();
                if( m_autoDelete && !m_querySent )
                    deleteLater();
            }
        }
    }

    void
    SpotifyQueryMaker::slotMemoryQueryDone()
    {
        DEBUG_BLOCK

        m_memoryQueryIsRunning = false;
        m_activeQueryCount--;

        if( m_activeQueryCount <= 0 )
        {
            emit queryDone();
            if( m_autoDelete  && !m_querySent )
                deleteLater();
        }
    }

    void
    SpotifyQueryMaker::runMemoryQueryAgain()
    {
        DEBUG_BLOCK

        if( m_memoryQueryMaker.data() )
            return;

        m_memoryQueryMaker = new MemoryQueryMaker( m_collection.data()->memoryCollection().toWeakRef(),
                                                   m_collection.data()->collectionId() );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::TrackList ) ),
                 this, SIGNAL( newResultReady( Meta::TrackList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::ArtistList ) ),
                 this, SIGNAL( newResultReady( Meta::ArtistList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::AlbumList ) ),
                 this, SIGNAL( newResultReady( Meta::AlbumList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::GenreList ) ),
                 this, SIGNAL( newResultReady( Meta::GenreList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::ComposerList ) ),
                 this, SIGNAL( newResultReady( Meta::ComposerList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::YearList ) ),
                 this, SIGNAL( newResultReady( Meta::YearList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::DataList ) ),
                 this, SIGNAL( newResultReady( Meta::DataList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( QStringList ) ),
                 this, SIGNAL( newResultReady( QStringList) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( newResultReady( Meta::LabelList ) ),
                 this, SIGNAL( newResultReady( Meta::LabelList ) ) );
        connect( m_memoryQueryMaker.data(), SIGNAL( queryDone() ),
                 this, SLOT( slotMemoryQueryDone() ) );
        m_memoryQueryMaker.data()->setAutoDelete( true );

        foreach( CurriedQMFunction *funPtr, m_queryMakerFunctions )
            (*funPtr)( m_memoryQueryMaker.data() );

        m_activeQueryCount++;
        m_memoryQueryIsRunning = true;
        m_memoryQueryMaker.data()->run();
    }
}

#include "SpotifyQueryMaker.moc"
