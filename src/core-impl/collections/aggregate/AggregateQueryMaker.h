/*
 *  Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AGGREGATEQUERYMAKER_H
#define AGGREGATEQUERYMAKER_H

#include "core/collections/QueryMaker.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/aggregate/AggregateMeta.h"

#include <QList>
#include <QMutex>
#include <QSet>

#include <KSharedPtr>

class CustomReturnFunction;
class CustomReturnValue;

namespace Collections
{

class AMAROK_EXPORT AggregateQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
        AggregateQueryMaker( Collections::AggregateCollection *collection, const QList<QueryMaker*> &queryMakers );
        ~AggregateQueryMaker();

        virtual void run();
        virtual void abortQuery();

        virtual QueryMaker* setQueryType( QueryType type );

        virtual QueryMaker* addReturnValue( qint64 value);
        virtual QueryMaker* addReturnFunction( ReturnFunction function, qint64 value );
        virtual QueryMaker* orderBy( qint64 value, bool descending = false );

        virtual QueryMaker* addMatch( const Meta::TrackPtr &track );
        virtual QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists );
        virtual QueryMaker* addMatch( const Meta::AlbumPtr &album );
        virtual QueryMaker* addMatch( const Meta::ComposerPtr &composer );
        virtual QueryMaker* addMatch( const Meta::GenrePtr &genre );
        virtual QueryMaker* addMatch( const Meta::YearPtr &year );
        virtual QueryMaker* addMatch( const Meta::LabelPtr &label );

        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );

        virtual QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );
        virtual QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );

        virtual QueryMaker* limitMaxResultSize( int size );

        virtual QueryMaker* beginAnd();
        virtual QueryMaker* beginOr();
        virtual QueryMaker* endAndOr();

        virtual QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );
        virtual QueryMaker* setLabelQueryMode( LabelQueryMode mode );

    private:
        void handleResult();

    private Q_SLOTS:
        void slotQueryDone();
        void slotNewTracksReady( const Meta::TrackList &tracks );
        void slotNewArtistsReady( const Meta::ArtistList &artists );
        void slotNewAlbumsReady( const Meta::AlbumList &albums );
        void slotNewGenresReady( const Meta::GenreList &genres );
        void slotNewComposersReady( const Meta::ComposerList &composers );
        void slotNewYearsReady( const Meta::YearList &years );
        void slotNewLabelsReady( const Meta::LabelList &labels );

    private:
        AggregateCollection *m_collection;
        QList<QueryMaker*> m_builders;
        int m_queryDoneCount;
        bool m_returnDataPointers;
        int m_maxResultSize;
        QueryType m_queryType;
        bool m_orderDescending;
        qint64 m_orderField;
        bool m_orderByNumberField;
        QMutex m_queryDoneCountMutex;
        // store AggregateCollection meta stuff using KSharedPtr,
        // otherwise AggregateCollection might delete it (as soon as it gets garbage collection)
        QSet<KSharedPtr<Meta::AggregateTrack> > m_tracks;
        QSet<KSharedPtr<Meta::AggregateArtist> > m_artists;
        QSet<KSharedPtr<Meta::AggregateAlbum> > m_albums;
        QSet<KSharedPtr<Meta::AggregateGenre> > m_genres;
        QSet<KSharedPtr<Meta::AggregateComposer> > m_composers;
        QSet<KSharedPtr<Meta::AggreagateYear> > m_years;
        QSet<KSharedPtr<Meta::AggregateLabel> > m_labels;
        QList<CustomReturnFunction*> m_returnFunctions;
        QList<CustomReturnValue*> m_returnValues;
};

}

#endif /* AGGREGATEQUERYMAKER_H */
