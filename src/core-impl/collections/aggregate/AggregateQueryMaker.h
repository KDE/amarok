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

#include "AmarokSharedPointer.h"

class CustomReturnFunction;
class CustomReturnValue;

namespace Collections
{

class AMAROK_EXPORT AggregateQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
        AggregateQueryMaker( Collections::AggregateCollection *collection, const QList<QueryMaker*> &queryMakers );
        ~AggregateQueryMaker() override;

        void run() override;
        void abortQuery() override;

        QueryMaker* setQueryType( QueryType type ) override;

        QueryMaker* addReturnValue( qint64 value) override;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) override;
        QueryMaker* orderBy( qint64 value, bool descending = false ) override;

        QueryMaker* addMatch( const Meta::TrackPtr &track ) override;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
        QueryMaker* addMatch( const Meta::AlbumPtr &album ) override;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer ) override;
        QueryMaker* addMatch( const Meta::GenrePtr &genre ) override;
        QueryMaker* addMatch( const Meta::YearPtr &year ) override;
        QueryMaker* addMatch( const Meta::LabelPtr &label ) override;

        QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;
        QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd ) override;

        QueryMaker* addNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;
        QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare ) override;

        QueryMaker* limitMaxResultSize( int size ) override;

        QueryMaker* beginAnd() override;
        QueryMaker* beginOr() override;
        QueryMaker* endAndOr() override;

        QueryMaker* setAlbumQueryMode( AlbumQueryMode mode ) override;
        QueryMaker* setLabelQueryMode( LabelQueryMode mode ) override;

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
        // store AggregateCollection meta stuff using AmarokSharedPointer,
        // otherwise AggregateCollection might delete it (as soon as it gets garbage collection)
        QSet<AmarokSharedPointer<Meta::AggregateTrack> > m_tracks;
        QSet<AmarokSharedPointer<Meta::AggregateArtist> > m_artists;
        QSet<AmarokSharedPointer<Meta::AggregateAlbum> > m_albums;
        QSet<AmarokSharedPointer<Meta::AggregateGenre> > m_genres;
        QSet<AmarokSharedPointer<Meta::AggregateComposer> > m_composers;
        QSet<AmarokSharedPointer<Meta::AggreagateYear> > m_years;
        QSet<AmarokSharedPointer<Meta::AggregateLabel> > m_labels;
        QList<CustomReturnFunction*> m_returnFunctions;
        QList<CustomReturnValue*> m_returnValues;
};

}

#endif /* AGGREGATEQUERYMAKER_H */
