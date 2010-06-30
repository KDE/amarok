/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNP_QUERYMAKER_H
#define UPNP_QUERYMAKER_H

#include "core/collections/QueryMaker.h"

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace KIO {
  class UDSEntry;
  typedef QList<UDSEntry> UDSEntryList;
  class Job;
  class ListJob;
}

class KJob;

namespace Collections {

class UpnpSearchCollection;

class UpnpQueryMaker : public QueryMaker
{
    Q_OBJECT

    public:
        UpnpQueryMaker( UpnpSearchCollection * );
        ~UpnpQueryMaker();

        QueryMaker* reset();
        void run() ;
        void abortQuery() ;

        QueryMaker* setQueryType( QueryType type ) ;
        QueryMaker* setReturnResultAsDataPtrs( bool resultAsDataPtrs ) ;
        QueryMaker* addReturnValue( qint64 value ) ;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) ;
        QueryMaker* orderBy( qint64 value, bool descending = false ) ;
        QueryMaker* orderByRandom() ;

        QueryMaker* includeCollection( const QString &collectionId ) ;
        QueryMaker* excludeCollection( const QString &collectionId ) ;

        QueryMaker* addMatch( const Meta::TrackPtr &track ) ;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist ) ;
        QueryMaker* addMatch( const Meta::AlbumPtr &album ) ;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer ) ;
        QueryMaker* addMatch( const Meta::GenrePtr &genre ) ;
        QueryMaker* addMatch( const Meta::YearPtr &year ) ;
        QueryMaker* addMatch( const Meta::DataPtr &data ) ;
        QueryMaker* addMatch( const Meta::LabelPtr &label );

        QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) ;
        QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false ) ;

        QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) ;
        QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare ) ;

        QueryMaker* limitMaxResultSize( int size ) ;

        QueryMaker* setAlbumQueryMode( AlbumQueryMode mode );

        QueryMaker* setLabelQueryMode( LabelQueryMode mode );

        QueryMaker* beginAnd() ;
        QueryMaker* beginOr() ;
        QueryMaker* endAndOr() ;

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

    private slots:
        void slotEntries( KIO::Job *, const KIO::UDSEntryList & );
        void slotDone( KJob * );
    private:
        void handleArtists( const KIO::UDSEntryList &list );
        void handleAlbums( const KIO::UDSEntryList &list );
        void handleTracks( const KIO::UDSEntryList &list );
        // TODO
        // this is all silly and crude and engineered
        // on an experimental purpose for the first queries
        // just to understand how it works.

        UpnpSearchCollection *m_collection;

        QueryType m_queryType;
        AlbumQueryMode m_albumMode;

        QString m_queryString;

        bool m_asDataPtrs;

        static bool m_runningJob;
        static int m_count;
        // TODO split this off into a class
        static QHash<QString, KIO::ListJob*> m_queries;

        bool m_noResults;
};

} //namespace Collections

#endif

