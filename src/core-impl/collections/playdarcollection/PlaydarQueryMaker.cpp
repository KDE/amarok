/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#include "PlaydarQueryMaker.h"

#include "PlaydarMeta.h"
#include "PlaydarCollection.h"

#include "support/Controller.h"
#include "support/Query.h"
#include "support/QMFunctionTypes.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/MemoryQueryMaker.h"

#include <QObject>
#include <QStack>

namespace Collections
{
    PlaydarQueryMaker::PlaydarQueryMaker( PlaydarCollection *collection )
    : m_queryType( QueryMaker::QueryType( None ) )
    , m_autoDelete( false )
    , m_activeQueryCount( 0 )
    , m_memoryQueryIsRunning( false )
    , m_collectionUpdated( false )
    , m_filterMap( )
    , m_collection( collection )
    , m_controller( new Playdar::Controller() )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = new MemoryQueryMaker( m_collection->memoryCollection().toWeakRef(),
                                                   m_collection->collectionId() );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newTracksReady,
                 this, &PlaydarQueryMaker::newTracksReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newArtistsReady,
                 this, &PlaydarQueryMaker::newArtistsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newAlbumsReady,
                 this, &PlaydarQueryMaker::newAlbumsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newGenresReady,
                 this, &PlaydarQueryMaker::newGenresReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newComposersReady,
                 this, &PlaydarQueryMaker::newComposersReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newYearsReady,
                 this, &PlaydarQueryMaker::newYearsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newDataReady,
                 this, &PlaydarQueryMaker::newDataReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newLabelsReady,
                 this, &PlaydarQueryMaker::newLabelsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newResultReady,
                 this, &PlaydarQueryMaker::newResultReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::queryDone,
                 this, &PlaydarQueryMaker::memoryQueryDone );
        m_memoryQueryMaker->setAutoDelete( true );
    }

    PlaydarQueryMaker::~PlaydarQueryMaker()
    {
        DEBUG_BLOCK

        if( !m_queryMakerFunctions.isEmpty() )
        {
            qDeleteAll( m_queryMakerFunctions.begin(), m_queryMakerFunctions.end() );
            m_queryMakerFunctions.clear();
        }
        
        delete m_memoryQueryMaker.data();
    }


    void
    PlaydarQueryMaker::run()
    {
        DEBUG_BLOCK
        
        if( !m_filterMap.isEmpty() )
        {
            connect( m_controller, &Playdar::Controller::playdarError,
                     this, &PlaydarQueryMaker::slotPlaydarError );
            connect( m_controller, &Playdar::Controller::queryReady,
                     this, &PlaydarQueryMaker::collectQuery );

            QString artist( "" );
            QString album( "" );
            QString title( "" );

            if( m_filterMap.contains( Meta::valArtist ) )
                artist.append( m_filterMap.value( Meta::valArtist ) );
            if( m_filterMap.contains( Meta::valAlbum ) )
                album.append( m_filterMap.value( Meta::valAlbum ) );
            if( m_filterMap.contains( Meta::valTitle ) )
                title.append( m_filterMap.value( Meta::valTitle ) );

            if( !artist.isEmpty() && !title.isEmpty() )
            {
                m_activeQueryCount++;
                m_controller->resolve( artist, album, title );
            }
        }

        m_activeQueryCount++;
        m_memoryQueryIsRunning = true;
        m_memoryQueryMaker->run();
    }

    void
    PlaydarQueryMaker::abortQuery()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker->abortQuery();

        m_controller->disconnect( this );
    }

    QueryMaker*
    PlaydarQueryMaker::setQueryType( QueryType type )
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
    PlaydarQueryMaker::addReturnValue( qint64 value )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< qint64 >::FunPtr funPtr = &QueryMaker::addReturnValue;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< qint64 >( funPtr, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< ReturnFunction, qint64 >::FunPtr funPtr = &QueryMaker::addReturnFunction;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< ReturnFunction, qint64 >( funPtr, function, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::orderBy( qint64 value, bool descending )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< qint64, bool >::FunPtr funPtr = &QueryMaker::orderBy;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< qint64, bool >( funPtr, value, descending );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }

    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::TrackPtr &track )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< const Meta::TrackPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::TrackPtr& >( funPtr, track );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }

    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction<const Meta::ArtistPtr &, ArtistMatchBehaviour>::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction<const Meta::ArtistPtr &, ArtistMatchBehaviour>( funPtr, artist, behaviour );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );

        if( artist )
            m_filterMap.insert( Meta::valArtist, artist->name() );

        return this;
    }

    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::AlbumPtr &album )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::AlbumPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::AlbumPtr& >( funPtr, album );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        if( album )
            m_filterMap.insert( Meta::valAlbum, album->name() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::ComposerPtr &composer )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::ComposerPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::ComposerPtr& >( funPtr, composer );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::GenrePtr &genre )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::GenrePtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::GenrePtr& >( funPtr, genre );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::YearPtr &year )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::YearPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::YearPtr& >( funPtr, year );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }

    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::LabelPtr &label )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::LabelPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::LabelPtr& >( funPtr, label );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK
        
        CurriedQMStringFilterFunction::FunPtr funPtr = &QueryMaker::addFilter;
        CurriedQMFunction *curriedFun =
            new CurriedQMStringFilterFunction( funPtr, value, filter, matchBegin, matchEnd );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        if( !m_filterMap.isEmpty() && m_filterMap.contains( value ) )
        {
            QString newFilter = m_filterMap.value( value );
            newFilter.append( QStringLiteral( " " ) ).append( filter );
            m_filterMap.insert( value, newFilter );
        }
        else
            m_filterMap.insert( value, filter );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
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
    PlaydarQueryMaker::addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
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
    PlaydarQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
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
    PlaydarQueryMaker::limitMaxResultSize( int size )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< int >::FunPtr funPtr = &QueryMaker::limitMaxResultSize;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< int >( funPtr, size );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< AlbumQueryMode >::FunPtr funPtr = &QueryMaker::setAlbumQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< AlbumQueryMode >( funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setLabelQueryMode( LabelQueryMode mode )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< LabelQueryMode >::FunPtr funPtr = &QueryMaker::setLabelQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< LabelQueryMode >( funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginAnd()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginAnd;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginOr()
    {
        DEBUG_BLOCK
        
        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::endAndOr()
    {
        DEBUG_BLOCK
        
        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::endAndOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( funPtr );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)( m_memoryQueryMaker.data() );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setAutoDelete( bool autoDelete )
    {
        DEBUG_BLOCK
        
        m_autoDelete = autoDelete;
        
        return this;
    }
    
    int
    PlaydarQueryMaker::validFilterMask()
    {
        DEBUG_BLOCK
        
        return QueryMaker::ValidFilters( ArtistFilter ) |
               QueryMaker::ValidFilters( AlbumFilter ) |
               QueryMaker::ValidFilters( TitleFilter ) |
               m_memoryQueryMaker->validFilterMask();
    }

    void
    PlaydarQueryMaker::slotPlaydarError( Playdar::Controller::ErrorState error )
    {
        DEBUG_BLOCK

        Q_EMIT playdarError( error );
    }
    
    void
    PlaydarQueryMaker::collectQuery( Playdar::Query* query )
    {
        DEBUG_BLOCK
        
        connect( query, &Playdar::Query::newTrackAdded,
                 this, &PlaydarQueryMaker::collectResult );
        connect( query, &Playdar::Query::queryDone,
                 this, &PlaydarQueryMaker::aQueryEnded );
    }

    void
    PlaydarQueryMaker::collectResult( Meta::PlaydarTrackPtr track )
    {
        DEBUG_BLOCK
        
        track->addToCollection( m_collection.data() );
        if( m_collection->trackForUrl( QUrl(track->uidUrl()) ) == Meta::TrackPtr::staticCast( track ) )
            m_collectionUpdated = true;
    }
    
    void
    PlaydarQueryMaker::aQueryEnded( Playdar::Query *query, const Meta::PlaydarTrackList &trackList )
    {
        DEBUG_BLOCK

        Q_UNUSED( query );
        Q_UNUSED( trackList );

        m_activeQueryCount--;

        
        if( m_activeQueryCount <= 0 )
        {
            if( m_collectionUpdated && !m_memoryQueryIsRunning )
            {
                m_collectionUpdated = false;
                runMemoryQueryAgain();
            }
            else
            {
                Q_EMIT queryDone();
                if( m_autoDelete )
                    deleteLater();
            }
        }
    }

    void
    PlaydarQueryMaker::memoryQueryDone()
    {
        DEBUG_BLOCK

        m_memoryQueryIsRunning = false;
        m_activeQueryCount--;

        if( m_activeQueryCount <= 0 )
        {
            Q_EMIT queryDone();
            if( m_autoDelete )
                deleteLater();
        }
    }

    void
    PlaydarQueryMaker::runMemoryQueryAgain()
    {
        DEBUG_BLOCK

        if( m_memoryQueryMaker.data() )
            return;
        
        m_memoryQueryMaker = new MemoryQueryMaker( m_collection->memoryCollection().toWeakRef(),
                                                   m_collection->collectionId() );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newTracksReady,
                 this, &PlaydarQueryMaker::newTracksReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newArtistsReady,
                 this, &PlaydarQueryMaker::newArtistsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newAlbumsReady,
                 this, &PlaydarQueryMaker::newAlbumsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newGenresReady,
                 this, &PlaydarQueryMaker::newGenresReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newComposersReady,
                 this, &PlaydarQueryMaker::newComposersReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newYearsReady,
                 this, &PlaydarQueryMaker::newYearsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newDataReady,
                 this, &PlaydarQueryMaker::newDataReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newResultReady,
                 this, &PlaydarQueryMaker::newResultReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::newLabelsReady,
                 this, &PlaydarQueryMaker::newLabelsReady );
        connect( m_memoryQueryMaker, &MemoryQueryMaker::queryDone,
                 this, &PlaydarQueryMaker::memoryQueryDone );
        m_memoryQueryMaker->setAutoDelete( true );

        for( CurriedQMFunction *funPtr : m_queryMakerFunctions )
            (*funPtr)( m_memoryQueryMaker.data() );
        
        m_activeQueryCount++;
        m_memoryQueryIsRunning = true;
        m_memoryQueryMaker.data()->run();
    }
}
