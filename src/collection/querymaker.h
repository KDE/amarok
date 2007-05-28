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
        //track metadata
        static const qint64 valUrl          = 1LL << 0;
        static const qint64 valTitle        = 1LL << 1;
        static const qint64 valArtist       = 1LL << 2;
        static const qint64 valAlbum        = 1LL << 3;
        static const qint64 valGenre        = 1LL << 4;
        static const qint64 valComposer     = 1LL << 5;
        static const qint64 valYear         = 1LL << 6;
        static const qint64 valComment      = 1LL << 7;
        static const qint64 valTrackNr      = 1LL << 8;
        static const qint64 valDiscNr       = 1LL << 9;
        //track data
        static const qint64 valLength       = 1LL << 10;
        static const qint64 valBitrate      = 1LL << 11;
        static const qint64 valSamplerate   = 1LL << 12;
        static const qint64 valFilesize     = 1LL << 13;
        static const qint64 valFormat       = 1LL << 14;
        //static const qint64 valCreateDate   = 1LL << 15;
        //statistics
        static const qint64 valScore        = 1LL << 16;
        static const qint64 valRating       = 1LL << 17;
        static const qint64 valFirstPlayed  = 1LL << 18;
        static const qint64 valLastPlayed   = 1LL << 19;
        static const qint64 valPlaycount    = 1LL << 20;

        QueryMaker();
        virtual ~QueryMaker();

        /**
            resets all internal data to the default values. Calling this method is the same
            as creating a new QueryMaker. Returns <code>this</code>
        */
        virtual QueryMaker* reset() = 0;
        /**
            starts the query. This method returns immediately. All processing is done in one or more
            separate worker thread(s). One of the newResultReady signals will be emitted at least once,
            followed by the queryDone() signal exactly once. See BlockingQuery for a way to run a query
            synchronously.
        */
        virtual void run() = 0;
        /**
            aborts a running query. Calling this method aborts a running query as soon as possible. This method returns immediately. No signals will be emitted after calling this method. This
            method has no effect if no query is running.
        */
        virtual void abortQuery() = 0;

        /**
            returns the number of times one of the newResultReady signals will be emitted
            by the QueryMaker instance.
         */
        virtual int resultCount() const;

        /**
            starts a query for Meta::Track objects. The results of the query will be returned as
            Meta::Track objects, so you have to connect your client to the
            newResultReady( QString, Meta::TrackList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startTrackQuery() = 0;
        /**
            starts a query for Meta::Artist objects. The results of the query will be returned as
            Meta::Artist objects, so you have to connect your client to the
            newResultReady( QString, Meta::ArtistList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startArtistQuery() = 0;
        /**
            starts a query for Meta::Album objects. The results of the query will be returned as
            Meta::Album objects, so you have to connect your client to the
            newResultReady( QString, Meta::AlbumList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startAlbumQuery() = 0;
        /**
            starts a query for Meta::Genre objects. The results of the query will be returned as
            Meta::Genre objects, so you have to connect your client to the
            newResultReady( QString, Meta::GenreList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startGenreQuery() = 0;
        /**
            starts a query for Meta::Composer objects. The results of the query will be returned as
            Meta::Composer objects, so you have to connect your client to the
            newResultReady( QString, Meta::ComposerList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startComposerQuery() = 0;
        /**
            starts a query for Meta::Year objects. The results of the query will be returned as
            Meta::Year objects, so you have to connect your client to the
            newResultReady( QString, Meta::YearList ) signal (unless you want the results as
            Meta::Data pointers, see returnResultAsDataPtrs( bool ) for details.
            Returns <code>this</code>
        */
        virtual QueryMaker* startYearQuery() = 0;
        /**
            starts a custom query. unlike the other startX methods, you have to set up the return
            values yourself using addReturnValue( qint64 ) and addReturnFunction(). The results will
            be returned as a QStringList. Threfore you have to connect to the
            newResultReady( QString, QStringList ) signal to receive the results. Calling
            returnResultsAsDataPtrs( bool ) has no effect when using a custom query.
            Returns <code>this</code>
        */
        virtual QueryMaker* startCustomQuery() = 0;
        /**
            sets the QueryMaker instance to return Meta::Data objects instead of the actual type.
            In some cases it can be useful to ignore the actual type of the result and just work with
            the method provided by Meta::Data. Calling this method with resultAsDataPtrs = true causes
            the QueryMaker instance to emit the newResultReady( QString, Meta::DataList ) signal
            for all query types (except a custom query). Calling the method with resultAsDataPtrs = false
            switches back to the normal behaviour. Returns <code>this</code>
        */
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

        virtual QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;
        virtual QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) = 0;

        virtual QueryMaker* limitMaxResultSize( int size ) = 0;

    signals:
        void newResultReady( QString collectionId, Meta::TrackList );
        void newResultReady( QString collectionId, Meta::ArtistList );
        void newResultReady( QString collectionId, Meta::AlbumList );
        void newResultReady( QString collectionId, Meta::GenreList );
        void newResultReady( QString collectionId, Meta::ComposerList );
        void newResultReady( QString collectionId, Meta::YearList );
        void newResultReady( QString collectionId, Meta::DataList );
        void newResultReady( QString collectionId, QStringList );

        void queryDone();
};

#endif /* AMAROK_COLLECTION_QUERYMAKER_H */

