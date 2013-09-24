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
#ifndef SPOTIFY_QUERYMAKER_H
#define SPOTIFY_QUERYMAKER_H

#include "SpotifyMeta.h"
#include "support/Controller.h"
#include "support/QMFunctionTypes.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"

#include <QMap>

namespace Spotify
{
    class Query;
}

namespace Collections
{
    class QueryMakerFunction;
    class SpotifyCollection;

    class SpotifyQueryMaker : public QueryMaker
    {
        Q_OBJECT

        public:
            SpotifyQueryMaker( SpotifyCollection *collection );
            ~SpotifyQueryMaker();

            void run();
            void abortQuery();

            QueryMaker* setQueryType( QueryType type );
            QueryMaker* addReturnValue( qint64 value );
            QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
            QueryMaker* orderBy( qint64 value, bool descending = false );

            QueryMaker* addMatch( const Meta::TrackPtr &track );
            QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists );
            QueryMaker* addMatch( const Meta::AlbumPtr &album );
            QueryMaker* addMatch( const Meta::ComposerPtr &composer );
            QueryMaker* addMatch( const Meta::GenrePtr &genre );
            QueryMaker* addMatch( const Meta::YearPtr &year );
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
            void newResultReady( Meta::TrackList );
            void newResultReady( Meta::ArtistList );
            void newResultReady( Meta::AlbumList );
            void newResultReady( Meta::GenreList );
            void newResultReady( Meta::ComposerList );
            void newResultReady( Meta::YearList );
            void newResultReady( QStringList );
            void newResultReady( Meta::LabelList );

            void queryDone();
            void queryAborted();
            void spotifyError( const Spotify::Controller::ErrorState );

        public Q_SLOTS:
            void slotSpotifyError( const Spotify::Controller::ErrorState error );
            void slotCollectResults( const Meta::SpotifyTrackList &track );
            void slotQueryDone( Spotify::Query* query, const Meta::SpotifyTrackList trackList );
            void slotMemoryQueryDone();

        private:
            QueryType m_queryType;
            bool m_autoDelete;
            int  m_activeQueryCount;
            bool m_memoryQueryIsRunning;
            bool m_collectionUpdated;
            bool m_querySent;
            QList< CurriedQMFunction* > m_queryMakerFunctions;

            typedef QMap< qint64, QString > FilterMap;
            FilterMap m_filterMap;

            QWeakPointer< SpotifyCollection > m_collection;
            QWeakPointer< QueryMaker > m_memoryQueryMaker;

            QWeakPointer< Spotify::Controller > m_controller;

            void runMemoryQueryAgain();
    };



}

#endif /* SPOTIFY_QUERYMAKER_H */
