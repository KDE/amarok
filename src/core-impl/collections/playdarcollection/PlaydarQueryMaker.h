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


#ifndef PLAYDAR_QUERYMAKER_H
#define PLAYDAR_QUERYMAKER_H

#include "PlaydarMeta.h"
#include "PlaydarCollection.h"

#include "support/Controller.h"
#include "support/Query.h"
#include "support/QMFunctionTypes.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/collections/QueryMaker.h"

#include <QMap>
#include <QStack>

namespace Collections
{
    class QueryMakerFunction;
    
    class PlaydarQueryMaker : public QueryMaker
    {
        Q_OBJECT
        
        public:
            PlaydarQueryMaker( PlaydarCollection *collection );
            ~PlaydarQueryMaker();
            
            QueryMaker* reset();
            void run();
            void abortQuery();
            
            int resultCount() const;
            
            QueryMaker* setQueryType( QueryType type );
            QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs );
            QueryMaker* addReturnValue( qint64 value );
            QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
            QueryMaker* orderBy( qint64 value, bool descending = false );
            QueryMaker* orderByRandom();
            
            QueryMaker* includeCollection( const QString &collectionId );
            QueryMaker* excludeCollection( const QString &collectionId );
            
            QueryMaker* addMatch( const Meta::TrackPtr &track );
            QueryMaker* addMatch( const Meta::ArtistPtr &artist );
            QueryMaker* addMatch( const Meta::AlbumPtr &album );
            QueryMaker* addMatch( const Meta::ComposerPtr &composer );
            QueryMaker* addMatch( const Meta::GenrePtr &genre );
            QueryMaker* addMatch( const Meta::YearPtr &year );
            QueryMaker* addMatch( const Meta::DataPtr &data );
            QueryMaker* addMatch( const Meta::LabelPtr &label );
            
            QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
            QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false );
            
            QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
            QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare );
            
            QueryMaker* limitMaxResultSize( int size );
            
            QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
            
            QueryMaker* setLabelQueryMode( LabelQueryMode mode );
            
            QueryMaker* beginAnd();
            QueryMaker* beginOr();
            QueryMaker* endAndOr();
            QueryMaker* setAutoDelete( bool autoDelete );
            
            int validFilterMask();
            
        signals:
            void newResultReady( QString collectionId, Meta::TrackList );
            void newResultReady( QString collectionId, Meta::ArtistList );
            void newResultReady( QString collectionId, Meta::AlbumList );
            void newResultReady( QString collectionId, Meta::GenreList );
            void newResultReady( QString collectionId, Meta::ComposerList );
            void newResultReady( QString collectionId, Meta::YearList );
            void newResultReady( QString collectionId, Meta::DataList );
            void newResultReady( QString collectionId, QStringList );
            void newResultReady( QString collectionId, Meta::LabelList );
            
            void queryDone();

            void playdarError( Playdar::Controller::ErrorState );
        
        private Q_SLOTS:
            void slotPlaydarError( Playdar::Controller::ErrorState error );
            void collectQuery( Playdar::Query* query );
            void collectResult( Meta::PlaydarTrackPtr track );
            void aQueryEnded( const Meta::PlaydarTrackList &trackList );
            void memoryQueryDone();
            
        private:
            bool m_autoDelete;
            bool m_shouldQueryCollection;
            int m_activeQueryCount;
            bool m_memoryQueryIsRunning;
            bool m_collectionUpdated;
            QList< CurriedQMFunction* > m_queryMakerFunctions;
            
            typedef QMap< qint64, QString > FilterMap;
            typedef QList< FilterMap* > FilterMapList;
            FilterMapList m_filterMapList;
            
            QPointer< PlaydarCollection > m_collection;
            QPointer< QueryMaker > m_memoryQueryMaker;
            
            QPointer< Playdar::Controller > m_controller;
            
            QStack< bool > m_andOrStack;
            
            void runMemoryQueryAgain();
    };

    

}

#endif
