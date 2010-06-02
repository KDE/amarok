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

        m_memoryQueryMaker = m_memoryQueryMaker->setQueryType( type );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setReturnResultAsDataPtrs( bool resultAsDataPtrs )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->setReturnResultAsDataPtrs( resultAsDataPtrs );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addReturnValue( qint64 value )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->addReturnValue( value );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addReturnFunction( ReturnFunction function, qint64 value )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->addReturnFunction( function, value );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::orderBy( qint64 value, bool descending )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->orderBy( value, descending );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::orderByRandom()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->orderByRandom();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::includeCollection( const QString &collectionId )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->includeCollection( collectionId );
        
        if( m_collection->collectionId() == collectionId )
            m_shouldQueryCollection = true;
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::excludeCollection( const QString &collectionId )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->excludeCollection( collectionId );
        
        if( m_collection->collectionId() == collectionId )
            m_shouldQueryCollection = false;
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::TrackPtr &track )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( track );
        
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

        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( artist );

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

        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( album );
        
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
        
        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( composer );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::GenrePtr &genre )
    {
        DEBUG_BLOCK
        
        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( genre );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addMatch( const Meta::YearPtr &year )
    {
        DEBUG_BLOCK
        
        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( year );
        
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
        
        m_memoryQueryMaker = m_memoryQueryMaker->addMatch( label );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        DEBUG_BLOCK
        
        m_memoryQueryMaker = m_memoryQueryMaker->addFilter( value, filter, matchBegin, matchEnd );
        
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
        
        m_memoryQueryMaker = m_memoryQueryMaker->excludeFilter( value, filter, matchBegin, matchEnd );
        
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
        
        m_memoryQueryMaker = m_memoryQueryMaker->addNumberFilter( value, filter, compare );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )
    {
        DEBUG_BLOCK
        
        m_memoryQueryMaker = m_memoryQueryMaker->excludeNumberFilter( value, filter, compare );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::limitMaxResultSize( int size )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->limitMaxResultSize( size );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->setAlbumQueryMode( mode );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setLabelQueryMode( LabelQueryMode mode )
    {
        DEBUG_BLOCK
        
        m_memoryQueryMaker = m_memoryQueryMaker->setLabelQueryMode( mode );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginAnd()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->beginAnd();
        
        m_andOrStack.push( true );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::beginOr()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->beginOr();
        
        m_andOrStack.push( false );
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::endAndOr()
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->endAndOr();
        
        if( !m_andOrStack.isEmpty() )
            m_andOrStack.pop();
        
        return this;
    }
    
    QueryMaker*
    PlaydarQueryMaker::setAutoDelete( bool autoDelete )
    {
        DEBUG_BLOCK

        m_memoryQueryMaker = m_memoryQueryMaker->setAutoDelete( autoDelete );
        
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
    }
    
    void
    PlaydarQueryMaker::aQueryEnded(Meta::PlaydarTrackList trackList)
    {
        DEBUG_BLOCK
        
        Q_UNUSED( trackList );

        m_activeQueryCount--;
        
        if( m_activeQueryCount <= 0 )
        {
            emit queryDone();
            if( m_autoDelete )
                deleteLater();
        }
    }

    void
    PlaydarQueryMaker::memoryQueryDone()
    {
        DEBUG_BLOCK
        
        m_activeQueryCount--;

        if( m_activeQueryCount <= 0 )
        {
            emit queryDone();
            if( m_autoDelete )
                deleteLater();
        }
    }

}