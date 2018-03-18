/*
    Copyright (C) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef UPNPQUERYMAKERINTERNAL_H
#define UPNPQUERYMAKERINTERNAL_H

#include <QObject>

#include <QUrl>
#include <kio/udsentry.h>

#include "core/collections/QueryMaker.h"

class KJob;
namespace KIO {
    class Job;
    class SimpleJob;
}

namespace Collections {

class UpnpSearchCollection;

class UpnpQueryMakerInternal : public QObject
{
    Q_OBJECT
    public:
        explicit UpnpQueryMakerInternal( UpnpSearchCollection *collection );
        ~UpnpQueryMakerInternal();
        void setQueryType( Collections::QueryMaker::QueryType type ) { m_queryType = type; }
        void reset();
        void runQuery( QUrl query, bool filter=true );

    Q_SIGNALS:
        void results( bool error, const KIO::UDSEntryList list );
        void done();

        void newTracksReady( Meta::TrackList );
        void newArtistsReady( Meta::ArtistList );
        void newAlbumsReady( Meta::AlbumList );
        void newGenresReady( Meta::GenreList );
        void newResultReady( const KIO::UDSEntryList & );
    private Q_SLOTS:
        void slotEntries( KIO::Job *, const KIO::UDSEntryList & );
        void slotDone( KJob * );
        void slotStatDone( KJob * );

    private:
        void handleArtists( const KIO::UDSEntryList &list );
        void handleAlbums( const KIO::UDSEntryList &list );
        void handleTracks( const KIO::UDSEntryList &list );
        void handleCustom( const KIO::UDSEntryList &list );

        void queueJob( KIO::SimpleJob *job );
        void runStat( const QString &id );

    private:
        UpnpSearchCollection *m_collection;

        QueryMaker::QueryType m_queryType;
        int m_jobCount;
};

}

#endif // UPNPQUERYMAKERINTERNAL_H
