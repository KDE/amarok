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
        explicit UpnpQueryMaker( UpnpSearchCollection * );
        ~UpnpQueryMaker();

        QueryMaker* reset();
        void run()  override;
        void abortQuery()  override;

        QueryMaker* setQueryType( QueryType type )  override;
        QueryMaker* addReturnValue( qint64 value )  override;
        QueryMaker* addReturnFunction( ReturnFunction function, qint64 value )  override;
        QueryMaker* orderBy( qint64 value, bool descending = false )  override;

        QueryMaker* addMatch( const Meta::TrackPtr &track )  override;
        QueryMaker* addMatch( const Meta::ArtistPtr &artist, ArtistMatchBehaviour behaviour = TrackArtists ) override;
        QueryMaker* addMatch( const Meta::AlbumPtr &album )  override;
        QueryMaker* addMatch( const Meta::ComposerPtr &composer )  override;
        QueryMaker* addMatch( const Meta::GenrePtr &genre )  override;
        QueryMaker* addMatch( const Meta::YearPtr &year )  override;
        QueryMaker* addMatch( const Meta::LabelPtr &label ) override;

        QueryMaker* addFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false )  override;
        QueryMaker* excludeFilter( qint64 value, const QString &filter, bool matchBegin = false, bool matchEnd = false )  override;

        QueryMaker* addNumberFilter( qint64 value, qint64 filter, NumberComparison compare )  override;
        QueryMaker* excludeNumberFilter( qint64 value, qint64 filter, NumberComparison compare )  override;

        QueryMaker* limitMaxResultSize( int size )  override;

        QueryMaker* setAlbumQueryMode( AlbumQueryMode mode ) override;

        QueryMaker* setLabelQueryMode( LabelQueryMode mode ) override;

        QueryMaker* beginAnd()  override;
        QueryMaker* beginOr()  override;
        QueryMaker* endAndOr()  override;

        QueryMaker* setAutoDelete( bool autoDelete );

        int validFilterMask() override;

    Q_SIGNALS:
        void newTracksReady( Meta::TrackList );
        void newArtistsReady( Meta::ArtistList );
        void newAlbumsReady( Meta::AlbumList );
        void newGenresReady( Meta::GenreList );
        void newComposersReady( Meta::ComposerList );
        void newYearsReady( Meta::YearList );
        void newResultReady( QStringList );
        void newLabelsReady( Meta::LabelList );

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

