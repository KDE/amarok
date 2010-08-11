/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com                              *
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
    : m_autoDelete( false )
    , m_shouldQueryCollection( true )
    , m_activeQueryCount( 0 )
    , m_memoryQueryIsRunning( false )
    , m_collectionUpdated( false )
    , m_filterMapList( )
    , m_collection( collection )
    , m_controller( new Playdar::Controller() )
    , m_andOrStack( )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = new MemoryQueryMaker( m_collection->memoryCollection().toWeakRef(),
                                                   m_collection->collectionId() );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::TrackList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::ArtistList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::ArtistList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::AlbumList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::GenreList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::GenreList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::ComposerList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::ComposerList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::YearList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::YearList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::DataList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::DataList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, QStringList ) ),
                 this, SIGNAL( newResultReady( QString, QStringList) ) );
        connect( m_memoryQueryMaker, SIGNAL( newResultReady( QString, Meta::LabelList ) ),
                 this, SIGNAL( newResultReady( QString, Meta::LabelList ) ) );
        connect( m_memoryQueryMaker, SIGNAL( queryDone() ),
                 this, SLOT( memoryQueryDone() ) );
    }

    PlaydarQueryMaker::~PlaydarQueryMaker()
    {
        DEBUG_BLOCK
        delete m_controller;
        delete m_memoryQueryMaker;
    }
    
    QueryMaker*
    PlaydarQueryMaker::reset()
    {
        DEBUG_BLOCK
        delete m_controller;
        m_controller = new Playdar::Controller();
        m_autoDelete = false;
        m_shouldQueryCollection = true;
        m_filterMapList.clear();
        m_andOrStack.clear();

        m_memoryQueryMaker = m_memoryQueryMaker->reset();
        
        return this;
    }
    
    void
    PlaydarQueryMaker::run()
    {
        DEBUG_BLOCK
        
        if( m_shouldQueryCollection && !m_filterMapList.isEmpty() )
        {
            connect( m_controller, SIGNAL( playdarError( Playdar::Controller::ErrorState ) ),
                     this, SLOT( slotPlaydarError( Playdar::Controller::ErrorState ) ) );
            connect( m_controller, SIGNAL( queryReady( Playdar::Query* ) ),
                    this, SLOT( collectQuery( Playdar::Query* ) ) );

            foreach( FilterMap *filterMap, m_filterMapList )
            {
                QString artist( "" );
                QString album( "" );
                QString title( "" );

                if( filterMap->contains( Meta::valArtist ) )
                    artist.append( filterMap->value( Meta::valArtist ) );
                if( filterMap->contains( Meta::valAlbum ) )
                    album.append( filterMap->value( Meta::valAlbum ) );
                if( filterMap->contains( Meta::valTitle ) )
                    title.append( filterMap->value( Meta::valTitle ) );

                delete filterMap;
                filterMap = 0;

                if( !artist.isEmpty() && !title.isEmpty() )
                {
                    m_activeQueryCount++;
                    m_controller->resolve( artist, album, title );
                }
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
    
    int
    PlaydarQueryMaker::resultCount() const
    {
        DEBUG_BLOCK
        
        return m_memoryQueryMaker->resultCount();
    }
    
    QueryMaker*
    PlaydarQueryMaker::setQueryType( QueryType type )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< QueryType >::FunPtr funPtr = &QueryMaker::setQueryType;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< QueryType >( m_memoryQueryMaker, funPtr, type );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< bool >::FunPtr funPtr = &QueryMaker::setReturnResultAsDataPtrs;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< bool >( m_memoryQueryMaker, funPtr, resultAsDataPtrs );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addReturnValue( qint64 value )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< qint64 >::FunPtr funPtr = &QueryMaker::addReturnValue;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< qint64 >( m_memoryQueryMaker, funPtr, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< ReturnFunction, qint64 >::FunPtr funPtr = &QueryMaker::addReturnFunction;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< ReturnFunction, qint64 >( m_memoryQueryMaker, funPtr, function, value );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::orderBy( qint64 value, bool descending )
    {
        DEBUG_BLOCK

        CurriedBinaryQMFunction< qint64, bool >::FunPtr funPtr = &QueryMaker::orderBy;
        CurriedQMFunction *curriedFun = new CurriedBinaryQMFunction< qint64, bool >( m_memoryQueryMaker, funPtr, value, descending );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::orderByRandom()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::orderByRandom;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( m_memoryQueryMaker, funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::includeCollection( const QString &collectionId )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< const QString& >::FunPtr funPtr = &QueryMaker::includeCollection;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const QString& >( m_memoryQueryMaker, funPtr, collectionId );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        if( m_collection->collectionId() == collectionId )
            m_shouldQueryCollection = true;
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::excludeCollection( const QString &collectionId )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< const QString& >::FunPtr funPtr = &QueryMaker::excludeCollection;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const QString& >( m_memoryQueryMaker, funPtr, collectionId );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        if( m_collection->collectionId() == collectionId )
            m_shouldQueryCollection = false;
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::TrackPtr &track )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< const Meta::TrackPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::TrackPtr& >( m_memoryQueryMaker, funPtr, track );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        m_filterMapList.clear();
        FilterMap *trackMatch = new FilterMap;

        if( track )
            trackMatch->insert( Meta::valTitle, track->name() );
        if( track->artist() )
            trackMatch->insert( Meta::valArtist, track->artist()->name() );
        if( track->album() )
            trackMatch->insert( Meta::valAlbum, track->album()->name() );

        m_filterMapList.append( trackMatch );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::ArtistPtr &artist )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::ArtistPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::ArtistPtr& >( m_memoryQueryMaker, funPtr, artist );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();

        if( artist )
        {
            foreach( FilterMap *filterMap, m_filterMapList )
                filterMap->insert( Meta::valArtist, artist->name() );
        }
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::AlbumPtr &album )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::AlbumPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::AlbumPtr& >( m_memoryQueryMaker, funPtr, album );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        if( album )
        {
            foreach( FilterMap *filterMap, m_filterMapList )
                filterMap->insert( Meta::valAlbum, album->name() );
        }
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::ComposerPtr &composer )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::ComposerPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::ComposerPtr& >( m_memoryQueryMaker, funPtr, composer );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::GenrePtr &genre )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::GenrePtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::GenrePtr& >( m_memoryQueryMaker, funPtr, genre );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::YearPtr &year )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::YearPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::YearPtr& >( m_memoryQueryMaker, funPtr, year );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::DataPtr &data )
    {
        DEBUG_BLOCK
        
        ( const_cast<Meta::DataPtr&>(data) )->addMatchTo( this );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::LabelPtr &label )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< const Meta::LabelPtr& >::FunPtr funPtr = &QueryMaker::addMatch;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< const Meta::LabelPtr& >( m_memoryQueryMaker, funPtr, label );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK
        
        CurriedQuarternaryQMFunction< qint64, const QString&, bool, bool >::FunPtr funPtr = &QueryMaker::addFilter;
        CurriedQMFunction *curriedFun =
            new CurriedQuarternaryQMFunction< qint64, const QString&, bool, bool >
            (
                m_memoryQueryMaker, funPtr, value, filter, matchBegin, matchEnd
            );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        if( !m_filterMapList.isEmpty() )
        {
            foreach( FilterMap *filterMap, m_filterMapList )
            {
                if( filterMap->contains( value ) )
                {
                    QString newFilter = filterMap->value( value );
                    newFilter.append( QString( " " ) ).append( filter );
                    filterMap->insert( value, newFilter );
                }
                else
                    filterMap->insert( value, filter );
            }
        }
        else
        {
            FilterMap* filterMapPtr = new FilterMap;
            filterMapPtr->insert( value, filter );
            m_filterMapList.append( filterMapPtr );
        }
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK
        
        CurriedQuarternaryQMFunction< qint64, const QString&, bool, bool >::FunPtr funPtr = &QueryMaker::excludeFilter;
        CurriedQMFunction *curriedFun =
            new CurriedQuarternaryQMFunction< qint64, const QString&, bool, bool >
            (
                m_memoryQueryMaker, funPtr, value, filter, matchBegin, matchEnd
            );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        foreach( FilterMap *filterMap, m_filterMapList )
        {
            if( filterMap->contains( value ) && filterMap->value( value ).contains( filter ) )
            {
                QString localFilter = filterMap->value( value );
                localFilter.remove( filter );
                filterMap->insert( value, localFilter );
            }
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
                m_memoryQueryMaker, funPtr, value, filter, compare
            );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
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
                m_memoryQueryMaker, funPtr, value, filter, compare
            );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::limitMaxResultSize( int size )
    {
        DEBUG_BLOCK

        CurriedUnaryQMFunction< int >::FunPtr funPtr = &QueryMaker::limitMaxResultSize;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< int >( m_memoryQueryMaker, funPtr, size );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< AlbumQueryMode >::FunPtr funPtr = &QueryMaker::setAlbumQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< AlbumQueryMode >( m_memoryQueryMaker, funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setLabelQueryMode( LabelQueryMode mode )
    {
        DEBUG_BLOCK
        
        CurriedUnaryQMFunction< LabelQueryMode >::FunPtr funPtr = &QueryMaker::setLabelQueryMode;
        CurriedQMFunction *curriedFun = new CurriedUnaryQMFunction< LabelQueryMode >( m_memoryQueryMaker, funPtr, mode );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginAnd()
    {
        DEBUG_BLOCK

        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginAnd;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( m_memoryQueryMaker, funPtr );
        m_queryMakerFunctions.append( curriedFun );

        (*curriedFun)();
        
        m_andOrStack.push( true );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginOr()
    {
        DEBUG_BLOCK
        
        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::beginOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( m_memoryQueryMaker, funPtr );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        m_andOrStack.push( false );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::endAndOr()
    {
        DEBUG_BLOCK
        
        CurriedZeroArityQMFunction::FunPtr funPtr = &QueryMaker::endAndOr;
        CurriedQMFunction *curriedFun = new CurriedZeroArityQMFunction( m_memoryQueryMaker, funPtr );
        m_queryMakerFunctions.append( curriedFun );
        
        (*curriedFun)();
        
        if( !m_andOrStack.isEmpty() )
            m_andOrStack.pop();
        
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

        emit playdarError( error );
    }
    
    void
    PlaydarQueryMaker::collectQuery( Playdar::Query* query )
    {
        DEBUG_BLOCK
        
        connect( query, SIGNAL( newTrackAdded( Meta::PlaydarTrackPtr ) ),
                 this, SLOT( collectResult( Meta::PlaydarTrackPtr ) ) );
        connect( query, SIGNAL( queryDone( Meta::PlaydarTrackList ) ),
                 this, SLOT( aQueryEnded( Meta::PlaydarTrackList ) ) );
    }

    void
    PlaydarQueryMaker::collectResult( Meta::PlaydarTrackPtr track )
    {
        DEBUG_BLOCK
        
        track->addToCollection( m_collection );
        if( m_collection->trackForUrl( track->uidUrl() ) == Meta::TrackPtr::staticCast( track ) )
            m_collectionUpdated = true;
    }
    
    void
    PlaydarQueryMaker::aQueryEnded( const Meta::PlaydarTrackList &trackList )
    {
        DEBUG_BLOCK
        
        Q_UNUSED( trackList );

        m_activeQueryCount--;

        
        if( m_activeQueryCount <= 0 )
        {
            if( m_collectionUpdated && !m_memoryQueryIsRunning )
            {
                m_collectionUpdated = false;
                m_activeQueryCount++;
                runMemoryQueryAgain();
            }
            else
            {
                emit queryDone();
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
            emit queryDone();
            if( m_autoDelete )
                deleteLater();
        }
    }

    void
    PlaydarQueryMaker::runMemoryQueryAgain()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker->reset();
        
        foreach( CurriedQMFunction *funPtr, m_queryMakerFunctions )
            (*funPtr)();

        m_memoryQueryIsRunning = true;
        m_memoryQueryMaker->run();
    }
}