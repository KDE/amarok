/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
#ifndef AMAROK_COLLECTION_QUERYMAKER_H
#define AMAROK_COLLECTION_QUERYMAKER_H

#include "amarok_export.h"
#include "meta.h"

#include <QObject>
#include <QStringList>
#include <QtGlobal>

using namespace Meta;

class AMAROK_EXPORT QueryMaker : public QObject
{
    Q_OBJECT

    public:
        static const qint64 valUrl      = 1LL << 0;
        static const qint64 valTitle    = 1LL << 1;
        static const qint64 valArtist   = 1LL << 2;
        static const qint64 valALbum    = 1LL << 3;
        static const qint64 valGenre    = 1LL << 4;
        static const qint64 valComposer = 1LL << 5;
        static const qint64 valYear     = 1LL << 6;

        QueryMaker();
        virtual ~QueryMaker();

        virtual QueryMaker* reset() = 0;
        virtual void run() = 0;
        virtual void abortQuery() = 0;

        virtual QueryMaker* startTrackQuery() = 0;
        virtual QueryMaker* startArtistQuery() = 0;
        virtual QueryMaker* startAlbumQuery() = 0;
        virtual QueryMaker* startGenreQuery() = 0;
        virtual QueryMaker* startComposerQuery() = 0;
        virtual QueryMaker* startYearQuery() = 0;
        virtual QueryMaker* startCustomQuery() = 0;

        virtual QueryMaker* returnResultAsDataPtrs( bool resultAsDataPtrs ) = 0;

        /**
            only works after starting a custom query with startCustomQuery()
          */
        virtual QueryMaker* addReturnValue( qint64 value ) = 0;
        virtual QueryMaker* orderBy( qint64 value, bool descending = false ) = 0;

        virtual QueryMaker* includeCollection( const QString &collectionId ) = 0;
        virtual QueryMaker* excludeCollection( const QString &collectionId ) = 0;

        virtual QueryMaker* addMatch( const TrackPtr &track ) = 0;
        virtual QueryMaker* addMatch( const ArtistPtr &artist ) = 0;
        virtual QueryMaker* addMatch( const AlbumPtr &album ) = 0;
        virtual QueryMaker* addMatch( const ComposerPtr &composer ) = 0;
        virtual QueryMaker* addMatch( const GenrePtr &genre ) = 0;
        virtual QueryMaker* addMatch( const YearPtr &year ) = 0;
        virtual QueryMaker* addMatch( const DataPtr &data ) = 0;

        virtual QueryMaker* addFilter( qint64 value, const QString &filter ) = 0;
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter ) = 0;

    signals:
        AMAROK_EXPORT void newResultReady( QString collectionId, TrackList );
        AMAROK_EXPORT void newResultReady( QString collectionId, ArtistList );
        AMAROK_EXPORT void newResultReady( QString collectionId, AlbumList );
        AMAROK_EXPORT void newResultReady( QString collectionId, GenreList );
        AMAROK_EXPORT void newResultReady( QString collectionId, ComposerList );
        AMAROK_EXPORT void newResultReady( QString collectionId, YearList );
        AMAROK_EXPORT void newResultReady( QString collectionId, DataList );
        AMAROK_EXPORT void newResultReady( QString collectionId, QStringList );

        AMAROK_EXPORT void queryDone();
};

#endif /* AMAROK_COLLECTION_QUERYMAKER_H */

