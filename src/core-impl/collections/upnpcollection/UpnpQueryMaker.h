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
#include <QStack>

#include "UpnpQuery.h"

namespace KIO {
  class UDSEntry;
  typedef QList<UDSEntry> UDSEntryList;
  class Job;
  class ListJob;
}

class KJob;

namespace Collections {

class UpnpSearchCollection;
class UpnpQueryMakerInternal;

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
        QueryMaker* addReturnValue( qint64 value ) ;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value ) ;
        QueryMaker* orderBy( qint64 value, bool descending = false ) ;

        QueryMaker* addMatch( const Meta::TrackPtr &track ) ;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists );
        QueryMaker* addMatch( const Meta::AlbumPtr &album ) ;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer ) ;
        QueryMaker* addMatch( const Meta::GenrePtr &genre ) ;
        QueryMaker* addMatch( const Meta::YearPtr &year ) ;
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

    Q_SIGNALS:
        void newResultReady( Meta::TrackList );
        void newResultReady( Meta::ArtistList );
        void newResultReady( Meta::AlbumList );
        void newResultReady( Meta::GenreList );
        void newResultReady( Meta::ComposerList );
        void newResultReady( Meta::YearList );
        void newResultReady( QStringList );
        void newResultReady( Meta::LabelList );

        void queryDone();

    private Q_SLOTS:
        void slotDone();
        void handleArtists( Meta::ArtistList );
        void handleAlbums( Meta::AlbumList );
        void handleTracks( Meta::TrackList );

    private:
        /*
        * apply numeric filters and such which UPnP doesn't handle.
        */
        bool postFilter( const KIO::UDSEntry& entry );

        QString propertyForValue( qint64 value );

        UpnpSearchCollection *m_collection;
        UpnpQueryMakerInternal *m_internalQM;

        QueryType m_queryType;
        AlbumQueryMode m_albumMode;

        bool m_asDataPtrs;

        UpnpQuery m_query;

        bool m_noResults;
        int m_jobCount;

        Meta::DataList m_cacheEntries;

        ReturnFunction m_returnFunction;
        qint64 m_returnValue;

        struct NumericFilter {
            qint64 type;
            qint64 value;
            NumberComparison compare;
        };
        QList<NumericFilter> m_numericFilters;
};

} //namespace Collections

#endif

